// Copyright (c) 2026 Andrea. All Rights Reserved.

#include "SDevMenu.h"
#include "DevMenuComponent.h"

#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace DevMenuStyle
{
	// sRGB values, converted to linear by FLinearColor(FColor).
	static const FLinearColor Panel    (FColor( 10,  12,  16, 214));
	static const FLinearColor Border   (FColor(205, 210, 220,  80));
	static const FLinearColor Header   (FColor(255, 255, 255,  14));
	static const FLinearColor Text     (FColor(226, 229, 234));
	static const FLinearColor TextHot  (FColor(255, 255, 255));
	static const FLinearColor Dim      (FColor(128, 135, 148));
	static const FLinearColor On       (FColor(118, 226, 128));
	static const FLinearColor Off      (FColor(196,  96,  96));
	static const FLinearColor Broken   (FColor(255,  92,  92));

	/** The menu font, built from the TTF files in Resources/Fonts. Null if the files are missing. */
	static TSharedPtr<const FCompositeFont> LoadPluginFont()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("RuntimeDevMenu"));
		if (!Plugin.IsValid()) return nullptr;

		const FString Directory = Plugin->GetBaseDir() / TEXT("Resources/Fonts");
		const FString Regular = Directory / TEXT("JetBrainsMono-Regular.ttf");
		const FString Bold = Directory / TEXT("JetBrainsMono-Bold.ttf");
		if (!IFileManager::Get().FileExists(*Regular) || !IFileManager::Get().FileExists(*Bold)) return nullptr;

		// File-based faces hold no UObjects, so a plain FCompositeFont needs no GC handling.
		TSharedRef<FCompositeFont> Font = MakeShared<FCompositeFont>();
		Font->DefaultTypeface.AppendFont(TEXT("Regular"), Regular, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		Font->DefaultTypeface.AppendFont(TEXT("Bold"), Bold, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		return Font;
	}
}

void SDevMenu::Construct(const FArguments& InArgs)
{
	Menu = InArgs._Menu;

	const float Size = Menu.IsValid() ? Menu->FontSize : 13.f;
	const float SmallSize = FMath::Max(8.f, Size - 3.f);

	if (const TSharedPtr<const FCompositeFont> Composite = DevMenuStyle::LoadPluginFont())
	{
		Font      = FSlateFontInfo(Composite, Size, TEXT("Regular"));
		FontBold  = FSlateFontInfo(Composite, Size, TEXT("Bold"));
		FontSmall = FSlateFontInfo(Composite, SmallSize, TEXT("Regular"));
	}
	else  // the engine's monospace font
	{
		Font = FontBold = FCoreStyle::GetDefaultFontStyle("Mono", Size);
		FontSmall = FCoreStyle::GetDefaultFontStyle("Mono", SmallSize);
	}

	SetVisibility(EVisibility::HitTestInvisible);  // never steals mouse or focus from the game
	SetCanTick(false);
	ForceVolatile(true);                           // repaint every frame: values can change at any time
}

int32 SDevMenu::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const UDevMenuComponent* M = Menu.Get();
	if (!M || !M->bOpen) return LayerId;

	using namespace DevMenuStyle;
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	auto TextWidth = [&Measure](const FString& String, const FSlateFontInfo& InFont) { return static_cast<float>(Measure->Measure(String, InFont).X); };

	auto Box = [&](float X, float Y, float W, float H, const FLinearColor& Color, int32 Layer)
	{
		FSlateDrawElement::MakeBox(Out, Layer,
			Geometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(FMath::RoundToFloat(X), FMath::RoundToFloat(Y)))),
			&WhiteBrush, ESlateDrawEffect::None, Color);
	};
	auto Label = [&](float X, float Y, const FString& String, const FSlateFontInfo& InFont, const FLinearColor& Color, int32 Layer)
	{
		const FVector2f Size(TextWidth(String, InFont) + 2.f, Measure->GetMaxCharacterHeight(InFont));
		FSlateDrawElement::MakeText(Out, Layer,
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(FMath::RoundToFloat(X), FMath::RoundToFloat(Y)))),
			String, InFont, ESlateDrawEffect::None, Color);
	};

	// ---------------------------------------------------------------- Layout
	const TArray<UDevMenuComponent::FRow>& Rows = M->Rows;
	const float Scale    = M->FontSize / 13.f;
	const float PadX     = 12.f * Scale;
	const float PadY     = 6.f * Scale;
	const float CharH    = Measure->GetMaxCharacterHeight(Font);
	const float SmallH   = Measure->GetMaxCharacterHeight(FontSmall);
	const float RowH     = CharH + 4.f * Scale;
	const float HeaderH  = CharH + 12.f * Scale;
	const bool  bFooter  = !M->FooterHint.IsEmpty();
	const float FooterH  = bFooter ? SmallH + 12.f * Scale : 0.f;
	const float CursorW  = TextWidth(TEXT("\u25B8 "), Font);
	const float StateW   = TextWidth(TEXT("missing"), FontSmall);
	const float BarW     = 2.f * Scale;

	float LabelsW = TextWidth(M->Title, FontBold) + TextWidth(TEXT("PAUSED"), FontBold) + 24.f * Scale - CursorW;
	for (const UDevMenuComponent::FRow& Row : Rows) LabelsW = FMath::Max(LabelsW, TextWidth(Row.Label, Font));
	if (bFooter) LabelsW = FMath::Max(LabelsW, TextWidth(M->FooterHint, FontSmall) - CursorW);

	const int32 Visible = Rows.IsEmpty() ? 1 : FMath::Min(FMath::Max(1, M->MaxVisibleRows), Rows.Num());
	const float W = FMath::Max(M->MinWidth, PadX * 2.f + CursorW + LabelsW + 24.f * Scale + StateW);
	const float H = HeaderH + PadY * 2.f + Visible * RowH + FooterH;

	const FVector2f Screen = Geometry.GetLocalSize();
	const FVector2f Offset(M->ScreenOffset);
	const float X0 = M->Anchor == EDevMenuAnchor::TopRight ? Screen.X - W - Offset.X : Offset.X;
	const float Y0 = Offset.Y;
	const float TextX = X0 + PadX + CursorW;
	const float RightX = X0 + W - PadX;

	const int32 L0 = LayerId, L1 = LayerId + 1, L2 = LayerId + 2;

	// ---------------------------------------------------------------- Panel
	Box(X0, Y0, W, H, Panel, L0);
	Box(X0, Y0, W, 1.f, Border, L1);
	Box(X0, Y0 + H - 1.f, W, 1.f, Border, L1);
	Box(X0, Y0, 1.f, H, Border, L1);
	Box(X0 + W - 1.f, Y0, 1.f, H, Border, L1);

	// ---------------------------------------------------------------- Header
	Box(X0 + 1.f, Y0 + 1.f, W - 2.f, HeaderH - 1.f, Header, L1);
	Box(X0, Y0 + HeaderH, W, 1.f, Border, L1);
	Label(X0 + PadX, Y0 + (HeaderH - CharH) * 0.5f, M->Title, FontBold, TextHot, L2);

	if (UGameplayStatics::IsGamePaused(M))
	{
		const FString Paused = TEXT("PAUSED");
		Label(RightX - TextWidth(Paused, FontBold), Y0 + (HeaderH - CharH) * 0.5f, Paused, FontBold, M->AccentColor, L2);
	}
	else if (!Rows.IsEmpty())
	{
		int32 Index = 0, Count = 0;
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			if (Rows[i].bSeparator) continue;
			++Count;
			if (i <= M->Selected) Index = Count;
		}
		const FString Counter = FString::Printf(TEXT("%d/%d"), Index, Count);
		Label(RightX - TextWidth(Counter, FontSmall), Y0 + (HeaderH - SmallH) * 0.5f, Counter, FontSmall, Dim, L2);
	}

	// ---------------------------------------------------------------- Rows
	const float RowsY = Y0 + HeaderH + PadY;
	const double Now = FPlatformTime::Seconds();

	if (Rows.IsEmpty())
	{
		Label(TextX, RowsY + 2.f * Scale, TEXT("No rows yet. Add them in Details > Entries."), FontSmall, Dim, L2);
	}

	for (int32 v = 0; v < Visible && Rows.IsValidIndex(M->ScrollOffset + v); ++v)
	{
		const int32 Index = M->ScrollOffset + v;
		const UDevMenuComponent::FRow& Row = Rows[Index];
		const float Y = RowsY + v * RowH;
		const float TextY = Y + (RowH - CharH) * 0.5f;

		if (Row.bSeparator)
		{
			float LineX = X0 + PadX;
			if (!Row.Label.IsEmpty())
			{
				Label(LineX, Y + (RowH - SmallH) * 0.5f, Row.Label, FontSmall, Dim, L2);
				LineX += TextWidth(Row.Label, FontSmall) + 8.f * Scale;
			}
			Box(LineX, Y + RowH * 0.5f, RightX - LineX, 1.f, Border, L1);
			continue;
		}

		const bool bSelected = Index == M->Selected;
		if (bSelected)
		{
			Box(X0 + 1.f, Y, W - 2.f, RowH, M->AccentColor.CopyWithNewOpacity(0.16f), L1);
			Box(X0 + 1.f, Y, 3.f * Scale, RowH, M->AccentColor, L1);
			Label(X0 + PadX, TextY, TEXT("\u25B8"), Font, M->AccentColor, L2);
		}
		if (Now < Row.FlashUntil)
		{
			Box(X0 + 1.f, Y, W - 2.f, RowH, On.CopyWithNewOpacity(0.22f), L1);
		}

		Label(TextX, TextY, Row.Label, Font, Row.bBroken ? Broken : (bSelected ? TextHot : Text), L2);

		if (Row.bBroken)
		{
			const FString Missing = TEXT("missing");
			Label(RightX - TextWidth(Missing, FontSmall), Y + (RowH - SmallH) * 0.5f, Missing, FontSmall, Broken, L2);
		}
		else if (Row.GetState)
		{
			const TOptional<bool> State = Row.GetState();
			const FString Value = !State.IsSet() ? TEXT("--") : (State.GetValue() ? TEXT("ON") : TEXT("OFF"));
			const FLinearColor Color = !State.IsSet() ? Dim : (State.GetValue() ? On : Off);
			Label(RightX - TextWidth(Value, Font), TextY, Value, Font, Color, L2);
		}
	}

	// ---------------------------------------------------------------- Scrollbar
	if (Rows.Num() > Visible)
	{
		const float TrackY = RowsY, TrackH = Visible * RowH;
		const float ThumbH = FMath::Max(8.f * Scale, TrackH * Visible / Rows.Num());
		const float ThumbY = TrackY + (TrackH - ThumbH) * M->ScrollOffset / FMath::Max(1, Rows.Num() - Visible);
		Box(X0 + W - BarW - 3.f, TrackY, BarW, TrackH, Border, L1);
		Box(X0 + W - BarW - 3.f, ThumbY, BarW, ThumbH, M->AccentColor, L2);
	}

	// ---------------------------------------------------------------- Footer
	if (bFooter)
	{
		const float FooterY = Y0 + H - FooterH;
		Box(X0, FooterY, W, 1.f, Border, L1);
		Label(X0 + PadX, FooterY + (FooterH - SmallH) * 0.5f, M->FooterHint, FontSmall, Dim, L2);
	}

	return L2 + 1;
}
