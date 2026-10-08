// Copyright (c) 2026 Andrea. All Rights Reserved.

#include "DevMenuComponent.h"
#include "SDevMenu.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogDevMenu, Log, All);

namespace DevMenu
{
	static const FString SelfName = TEXT("Self");
	static const FString FunctionSuffix = TEXT("()");
	static constexpr int32 ViewportZOrder = 10000;
	static constexpr double FlashSeconds = 0.18;

	/**
	 * By default Enhanced Input ignores every held button after a context change until it is released.
	 * Turned off, so a held sprint or aim button keeps working when the menu opens or closes.
	 */
	static FModifyContextOptions ContextOptions()
	{
		FModifyContextOptions Options;
		Options.bIgnoreAllPressedKeysUntilRelease = false;
		return Options;
	}

	/** Classes declared by the engine or engine plugins: hidden from the dropdown by default. */
	static bool IsEngineClass(const UClass* Class, TMap<const UClass*, bool>& Cache)
	{
#if WITH_EDITOR
		if (const bool* Cached = Cache.Find(Class)) return *Cached;

		bool bEngine = false;
		const FString Package = Class->GetOutermost()->GetName();
		if (Package.StartsWith(TEXT("/Script/")))  // native class; Blueprints live in /Game or plugin content
		{
			const FString File = FModuleManager::Get().GetModuleFilename(FName(*Package.RightChop(8)));
			bEngine = !File.IsEmpty() && FPaths::IsUnderDirectory(File, FPaths::EngineDir());
		}
		return Cache.Add(Class, bEngine);
#else
		return false;
#endif
	}

	/** "Prefix.bBool" and "Prefix.Function()" for every member of Class the menu can use. */
	static void AppendMembers(const UClass* Class, const FString& Prefix, bool bIncludeEngine,
		TMap<const UClass*, bool>& EngineCache, TArray<FString>& Out)
	{
		for (TFieldIterator<FBoolProperty> It(Class); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Deprecated | CPF_EditorOnly)) continue;
			if (!bIncludeEngine && IsEngineClass(It->GetOwnerClass(), EngineCache)) continue;
			Out.Add(Prefix + TEXT(".") + It->GetName());
		}

		for (TFieldIterator<UFunction> It(Class); It; ++It)
		{
			const UFunction* Function = *It;
			if (Function->NumParms > 0) continue;                                   // no parameters, no return value
			if (Function->GetSuperFunction()) continue;                             // overrides: BeginPlay, Tick, construction script...
			if (Function->HasAnyFunctionFlags(FUNC_Static | FUNC_Delegate)) continue;
			if (!bIncludeEngine && IsEngineClass(Function->GetOwnerClass(), EngineCache)) continue;
			Out.Add(Prefix + TEXT(".") + Function->GetName() + FunctionSuffix);
		}
	}
}

UDevMenuComponent::UDevMenuComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;  // the menu has to work while the game is paused
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	ToggleKeys  = { EKeys::Gamepad_Special_Left, EKeys::F10 };
	ConfirmKeys = { EKeys::Gamepad_FaceButton_Bottom, EKeys::Enter };
	UpKeys      = { EKeys::Up };
	DownKeys    = { EKeys::Down };
	AccentColor = FLinearColor(FColor(255, 176, 32));
}

// ============================================================================ Lifecycle

void UDevMenuComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsAllowed())
	{
		SetComponentTickEnabled(false);
		return;
	}

	CreateInputObjects();
	ResolveEditorEntries();
}

void UDevMenuComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void UDevMenuComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The controller can arrive after BeginPlay (possession) or change later: follow it.
	APlayerController* PC = FindLocalPlayerController();
	if (PC != BoundController.Get())
	{
		Unbind();
		if (PC) Bind(PC);
	}
	if (!BoundController.IsValid()) return;

	KeepContextsApplied();
	if (bOpen)
	{
		if (bConfirmLocked && !IsAnyKeyDown(ConfirmKeys)) bConfirmLocked = false;
		UpdateNavigation();
	}
}

bool UDevMenuComponent::IsAllowed() const
{
	if (GetNetMode() == NM_DedicatedServer) return false;
#if UE_BUILD_SHIPPING
	return bEnableInShipping;
#else
	return true;
#endif
}

APlayerController* UDevMenuComponent::FindLocalPlayerController() const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		if (const APawn* Pawn = Cast<APawn>(GetOwner())) PC = Cast<APlayerController>(Pawn->GetController());
	}
	return PC && PC->IsLocalController() ? PC : nullptr;
}

// ============================================================================ Input

bool UDevMenuComponent::IsAnyKeyDown(const TArray<FKey>& Keys) const
{
	const APlayerController* PC = BoundController.Get();
	if (!PC) return false;
	for (const FKey& Key : Keys)
	{
		if (PC->IsInputKeyDown(Key)) return true;
	}
	return false;
}

void UDevMenuComponent::CreateInputObjects()
{
	// Actions and contexts are built in code: nothing to import, nothing to assign in the editor.
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = ValueType;
		Action->bConsumeInput = true;        // hides the key from lower-priority (gameplay) contexts
		Action->bTriggerWhenPaused = true;
		return Action;
	};

	ToggleAction     = MakeAction(TEXT("IA_DevMenu_Toggle"),     EInputActionValueType::Boolean);
	ConfirmAction    = MakeAction(TEXT("IA_DevMenu_Confirm"),    EInputActionValueType::Boolean);
	UpAction         = MakeAction(TEXT("IA_DevMenu_Up"),         EInputActionValueType::Boolean);
	DownAction       = MakeAction(TEXT("IA_DevMenu_Down"),       EInputActionValueType::Boolean);
	StickAction      = MakeAction(TEXT("IA_DevMenu_Stick"),      EInputActionValueType::Axis2D);
	StickBlockAction = MakeAction(TEXT("IA_DevMenu_StickBlock"), EInputActionValueType::Axis1D);

	// Always on: only the toggle.
	ToggleContext = NewObject<UInputMappingContext>(this, TEXT("IMC_DevMenu_Toggle"));
	for (const FKey& Key : ToggleKeys) ToggleContext->MapKey(ToggleAction, Key);

	// Only while open: everything the menu takes away from gameplay.
	MenuContext = NewObject<UInputMappingContext>(this, TEXT("IMC_DevMenu_Open"));
	for (const FKey& Key : ConfirmKeys) MenuContext->MapKey(ConfirmAction, Key);
	for (const FKey& Key : UpKeys)      MenuContext->MapKey(UpAction, Key);
	for (const FKey& Key : DownKeys)    MenuContext->MapKey(DownAction, Key);
	if (bUseRightStick)
	{
		MenuContext->MapKey(StickAction, EKeys::Gamepad_Right2D);
		// Games bind the camera either to the 2D stick or to its two axes: block both.
		MenuContext->MapKey(StickBlockAction, EKeys::Gamepad_RightX);
		MenuContext->MapKey(StickBlockAction, EKeys::Gamepad_RightY);
	}
}

void UDevMenuComponent::Bind(APlayerController* PC)
{
	ULocalPlayer* Player = PC->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		if (!bWarnedNoEnhancedInput)
		{
			UE_LOG(LogDevMenu, Warning, TEXT("Dev Menu: Enhanced Input is not active for this player, the menu is disabled."));
			bWarnedNoEnhancedInput = true;
		}
		return;
	}

	// Owned by the controller, never by the pawn: the controller builds its input stack from the pawn's
	// components AND from pushed components, so a pawn-owned one would be in the stack twice and every
	// binding would fire twice (open + close in the same frame).
	MenuInput = NewObject<UEnhancedInputComponent>(PC);
	MenuInput->RegisterComponent();
	MenuInput->BindAction(ToggleAction, ETriggerEvent::Started, this, &UDevMenuComponent::ToggleMenu);
	MenuInput->BindAction(ConfirmAction, ETriggerEvent::Started, this, &UDevMenuComponent::ExecuteSelected);
	MenuInput->BindActionValue(UpAction);
	MenuInput->BindActionValue(DownAction);
	MenuInput->BindActionValue(StickAction);
	PC->PushInputComponent(MenuInput);

	Subsystem->AddMappingContext(ToggleContext, InputPriority, DevMenu::ContextOptions());

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Widget = SNew(SDevMenu).Menu(this);
		Viewport->AddViewportWidgetForPlayer(Player, Widget.ToSharedRef(), DevMenu::ViewportZOrder);
	}

	BoundController = PC;
	BoundPlayer = Player;
	UE_LOG(LogDevMenu, Log, TEXT("Dev Menu ready on %s (%d rows)."), *PC->GetName(), Rows.Num());
}

void UDevMenuComponent::Unbind()
{
	ULocalPlayer* Player = BoundPlayer.Get();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		Subsystem->RemoveMappingContext(ToggleContext, DevMenu::ContextOptions());
		Subsystem->RemoveMappingContext(MenuContext, DevMenu::ContextOptions());
	}
	if (APlayerController* PC = BoundController.Get())
	{
		if (MenuInput) PC->PopInputComponent(MenuInput);
	}
	if (IsValid(MenuInput)) MenuInput->DestroyComponent();
	MenuInput = nullptr;
	if (Widget.IsValid())
	{
		UWorld* World = GetWorld();
		UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
		if (Viewport && Player) Viewport->RemoveViewportWidgetForPlayer(Player, Widget.ToSharedRef());
		Widget.Reset();
	}

	BoundController = nullptr;
	BoundPlayer = nullptr;
	bOpen = false;
}

void UDevMenuComponent::KeepContextsApplied()
{
	// Some games clear all mapping contexts on respawn or level change: put ours back.
	ULocalPlayer* Player = BoundPlayer.Get();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem) return;

	if (!Subsystem->HasMappingContext(ToggleContext)) Subsystem->AddMappingContext(ToggleContext, InputPriority, DevMenu::ContextOptions());
	if (bOpen && !Subsystem->HasMappingContext(MenuContext)) Subsystem->AddMappingContext(MenuContext, InputPriority, DevMenu::ContextOptions());
}

void UDevMenuComponent::UpdateNavigation()
{
	if (!MenuInput) return;

	float Vertical = 0.f;
	if (bUseRightStick)
	{
		const float StickY = static_cast<float>(MenuInput->GetBoundActionValue(StickAction).Get<FVector2D>().Y);
		Vertical += bInvertStickY ? -StickY : StickY;
	}
	if (MenuInput->GetBoundActionValue(UpAction).Get<bool>())   Vertical += 1.f;
	if (MenuInput->GetBoundActionValue(DownAction).Get<bool>()) Vertical -= 1.f;

	// Stick up = previous row.
	const int32 Direction = Vertical > StickThreshold ? -1 : (Vertical < -StickThreshold ? 1 : 0);
	if (Direction == 0)
	{
		HeldDirection = 0;
		return;
	}

	// Real time, not game time: works while paused or in slow motion.
	const double Now = FPlatformTime::Seconds();
	if (Direction != HeldDirection)
	{
		HeldDirection = Direction;
		NextRepeatTime = Now + RepeatDelay;
		MoveSelection(Direction);
	}
	else if (Now >= NextRepeatTime)
	{
		NextRepeatTime = Now + RepeatInterval;
		MoveSelection(Direction);
	}
}

// ============================================================================ Menu

void UDevMenuComponent::OpenMenu()
{
	if (bOpen || !BoundPlayer.IsValid()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = BoundPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(MenuContext, InputPriority, DevMenu::ContextOptions());
	}
	bOpen = true;
	HeldDirection = 0;
	bConfirmLocked = IsAnyKeyDown(ConfirmKeys);  // A already held (e.g. jumping): don't run a row on open
	EnsureValidSelection();
}

void UDevMenuComponent::CloseMenu()
{
	if (!bOpen) return;

	if (ULocalPlayer* Player = BoundPlayer.Get())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->RemoveMappingContext(MenuContext, DevMenu::ContextOptions());
		}
	}
	bOpen = false;
}

void UDevMenuComponent::ToggleMenu()
{
	if (bOpen) CloseMenu();
	else OpenMenu();
}

void UDevMenuComponent::MoveSelection(int32 Direction)
{
	const int32 Num = Rows.Num();
	if (Num == 0) return;

	for (int32 Step = 0; Step < Num; ++Step)  // wraps around and skips separators
	{
		Selected = (Selected + Direction + Num) % Num;
		if (!Rows[Selected].bSeparator) break;
	}
	ClampScroll();
}

void UDevMenuComponent::EnsureValidSelection()
{
	if (Rows.IsEmpty()) return;
	Selected = FMath::Clamp(Selected, 0, Rows.Num() - 1);
	if (Rows[Selected].bSeparator)
	{
		Selected = (Selected - 1 + Rows.Num()) % Rows.Num();
		MoveSelection(1);
	}
	ClampScroll();
}

void UDevMenuComponent::ClampScroll()
{
	const int32 Visible = FMath::Max(1, MaxVisibleRows);
	if (Selected < ScrollOffset) ScrollOffset = Selected;
	if (Selected >= ScrollOffset + Visible) ScrollOffset = Selected - Visible + 1;

	// Keep the header of the selected row's group in view.
	if (Selected > 0 && Rows.IsValidIndex(Selected - 1) && Rows[Selected - 1].bSeparator && Selected - 1 < ScrollOffset)
	{
		ScrollOffset = Selected - 1;
	}
	ScrollOffset = FMath::Clamp(ScrollOffset, 0, FMath::Max(0, Rows.Num() - Visible));
}

void UDevMenuComponent::ExecuteSelected()
{
	if (!bOpen || bConfirmLocked || !Rows.IsValidIndex(Selected)) return;

	FRow& Row = Rows[Selected];
	if (Row.bSeparator || !Row.Execute) return;

	Row.Execute();
	Row.FlashUntil = FPlatformTime::Seconds() + DevMenu::FlashSeconds;
	UE_LOG(LogDevMenu, Verbose, TEXT("Dev Menu: %s"), *Row.Label);
}

// ============================================================================ Rows

void UDevMenuComponent::AddToggle(const FString& Label, UObject* Target, FName BoolName)
{
	Rows.Add(MakeToggleRow(Label.IsEmpty() ? FName::NameToDisplayString(BoolName.ToString(), true) : Label, Target, BoolName));
}

void UDevMenuComponent::AddFunction(const FString& Label, UObject* Target, FName FunctionName)
{
	Rows.Add(MakeFunctionRow(Label.IsEmpty() ? FName::NameToDisplayString(FunctionName.ToString(), false) : Label, Target, FunctionName));
}

void UDevMenuComponent::AddAction(const FString& Label, FDevMenuAction Action)
{
	FRow Row;
	Row.Label = Label;
	Row.Execute = [Action]() { Action.ExecuteIfBound(); };
	Rows.Add(MoveTemp(Row));
}

void UDevMenuComponent::AddSeparator(const FString& Label)
{
	Rows.Add(MakeSeparatorRow(Label));
}

void UDevMenuComponent::AddLambda(const FString& Label, TFunction<void()> Execute, TFunction<bool()> GetState)
{
	FRow Row;
	Row.Label = Label;
	Row.Execute = MoveTemp(Execute);
	if (GetState)
	{
		Row.GetState = [GetState = MoveTemp(GetState)]() -> TOptional<bool> { return GetState(); };
	}
	Rows.Add(MoveTemp(Row));
}

void UDevMenuComponent::ClearRuntimeRows()
{
	Rows.SetNum(EditorRowCount);
	EnsureValidSelection();
}

void UDevMenuComponent::ResolveEditorEntries()
{
	TArray<FRow> Resolved;
	for (const FDevMenuEntry& Entry : Entries)
	{
		switch (Entry.Type)
		{
		case EDevMenuEntryType::Separator:
			Resolved.Add(MakeSeparatorRow(Entry.Label));
			break;

		case EDevMenuEntryType::PauseGame:
			Resolved.Add(MakePauseRow(Entry.Label.IsEmpty() ? TEXT("Pause Game") : Entry.Label));
			break;

		case EDevMenuEntryType::Binding:
		{
			// "Object.Member" or "Object.Member()"
			FString ObjectName, Member;
			if (!Entry.Binding.Split(TEXT("."), &ObjectName, &Member))
			{
				FRow Broken;
				Broken.Label = Entry.Label.IsEmpty() ? TEXT("(no binding)") : Entry.Label;
				Broken.bBroken = true;
				Resolved.Add(MoveTemp(Broken));
				break;
			}

			const bool bFunction = Member.RemoveFromEnd(DevMenu::FunctionSuffix);
			const FString Label = Entry.Label.IsEmpty() ? FName::NameToDisplayString(Member, !bFunction) : Entry.Label;
			UObject* Target = ResolveObject(ObjectName);
			Resolved.Add(bFunction ? MakeFunctionRow(Label, Target, FName(*Member)) : MakeToggleRow(Label, Target, FName(*Member)));
			break;
		}
		}
	}

	// Details panel rows first, runtime rows (AddToggle, AddAction...) after.
	EditorRowCount = Resolved.Num();
	Rows.Insert(MoveTemp(Resolved), 0);
	EnsureValidSelection();
}

UObject* UDevMenuComponent::ResolveObject(const FString& Name) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;
	if (Name == DevMenu::SelfName) return Owner;

	// Component variable (C++ UPROPERTY or Blueprint component)...
	if (const FObjectProperty* Property = FindFProperty<FObjectProperty>(Owner->GetClass(), FName(*Name)))
	{
		if (UObject* Object = Property->GetObjectPropertyValue_InContainer(Owner)) return Object;
	}
	// ...or any component with that name.
	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component && Component->GetName() == Name) return Component;
	}
	return nullptr;
}

UDevMenuComponent::FRow UDevMenuComponent::MakeToggleRow(const FString& Label, UObject* Target, FName BoolName) const
{
	FRow Row;
	Row.Label = Label;

	FBoolProperty* Property = Target ? FindFProperty<FBoolProperty>(Target->GetClass(), BoolName) : nullptr;
	if (!Property)
	{
		Row.bBroken = true;
		UE_LOG(LogDevMenu, Warning, TEXT("Dev Menu: bool '%s' not found on '%s'."), *BoolName.ToString(), *GetNameSafe(Target));
		return Row;
	}

	TWeakObjectPtr<UObject> Weak(Target);
	Row.Execute = [Weak, Property]()
	{
		if (UObject* Object = Weak.Get()) Property->SetPropertyValue_InContainer(Object, !Property->GetPropertyValue_InContainer(Object));
	};
	Row.GetState = [Weak, Property]() -> TOptional<bool>
	{
		if (const UObject* Object = Weak.Get()) return Property->GetPropertyValue_InContainer(Object);
		return {};
	};
	return Row;
}

UDevMenuComponent::FRow UDevMenuComponent::MakeFunctionRow(const FString& Label, UObject* Target, FName FunctionName) const
{
	FRow Row;
	Row.Label = Label;

	UFunction* Function = Target ? Target->FindFunction(FunctionName) : nullptr;
	if (!Function || Function->NumParms > 0)
	{
		Row.bBroken = true;
		UE_LOG(LogDevMenu, Warning, TEXT("Dev Menu: function '%s' not found on '%s', or it has parameters."), *FunctionName.ToString(), *GetNameSafe(Target));
		return Row;
	}

	TWeakObjectPtr<UObject> Weak(Target);
	Row.Execute = [Weak, Function]()
	{
		if (UObject* Object = Weak.Get()) Object->ProcessEvent(Function, nullptr);
	};
	return Row;
}

UDevMenuComponent::FRow UDevMenuComponent::MakePauseRow(const FString& Label) const
{
	FRow Row;
	Row.Label = Label;

	TWeakObjectPtr<const UDevMenuComponent> Weak(this);
	Row.Execute = [Weak]()
	{
		if (const UDevMenuComponent* Menu = Weak.Get()) UGameplayStatics::SetGamePaused(Menu, !UGameplayStatics::IsGamePaused(Menu));
	};
	Row.GetState = [Weak]() -> TOptional<bool>
	{
		if (const UDevMenuComponent* Menu = Weak.Get()) return UGameplayStatics::IsGamePaused(Menu);
		return {};
	};
	return Row;
}

UDevMenuComponent::FRow UDevMenuComponent::MakeSeparatorRow(const FString& Label)
{
	FRow Row;
	Row.Label = Label;
	Row.bSeparator = true;
	return Row;
}

// ============================================================================ Editor dropdown

TArray<FString> UDevMenuComponent::GetBindableMembers() const
{
	TArray<FString> Options;

	// On a placed actor the owner exists; on a Blueprint component template the outer is the Blueprint class.
	const UClass* OwnerClass = GetOwner() ? GetOwner()->GetClass() : GetTypedOuter<UClass>();
	if (!OwnerClass) return Options;

	TMap<const UClass*, bool> EngineCache;
	DevMenu::AppendMembers(OwnerClass, DevMenu::SelfName, bListEngineMembers, EngineCache, Options);

	// Every component variable of the owner, C++ or Blueprint, sorted by name.
	TArray<const FObjectProperty*> Components;
	for (TFieldIterator<FObjectProperty> It(OwnerClass); It; ++It)
	{
		const UClass* ComponentClass = It->PropertyClass;
		if (ComponentClass && ComponentClass->IsChildOf<UActorComponent>() && !ComponentClass->IsChildOf<UDevMenuComponent>())
		{
			Components.Add(*It);
		}
	}
	Components.Sort([](const FObjectProperty& A, const FObjectProperty& B) { return A.GetName() < B.GetName(); });

	for (const FObjectProperty* Component : Components)
	{
		DevMenu::AppendMembers(Component->PropertyClass, Component->GetName(), bListEngineMembers, EngineCache, Options);
	}
	return Options;
}