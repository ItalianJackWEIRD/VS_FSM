// Copyright (c) 2026 Andrea. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Brushes/SlateColorBrush.h"
#include "Fonts/SlateFontInfo.h"

class UDevMenuComponent;

/**
 * Draws the Dev Menu over the game. It never takes focus or mouse input:
 * input arrives through Enhanced Input in UDevMenuComponent, this widget only paints.
 */
class SDevMenu : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDevMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDevMenuComponent>, Menu)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

private:
	TWeakObjectPtr<UDevMenuComponent> Menu;
	FSlateFontInfo Font;
	FSlateFontInfo FontBold;
	FSlateFontInfo FontSmall;
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);
};
