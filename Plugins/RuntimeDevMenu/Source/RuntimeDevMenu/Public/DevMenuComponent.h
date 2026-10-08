// Copyright (c) 2026 Andrea. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "DevMenuTypes.h"
#include "DevMenuComponent.generated.h"

class APlayerController;
class ULocalPlayer;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class SDevMenu;

/**
 * In-game developer menu.
 *
 *  1. Add "Dev Menu" to your player Character or PlayerController.
 *  2. In Details > Entries, add rows and pick a bool or a function from the Binding dropdown.
 *  3. In game, press Select (gamepad) or F10 (keyboard).
 *
 * While open, the menu takes only the right stick and the confirm button.
 * Movement and every other input keep working, and the game never pauses unless a row asks for it.
 */
UCLASS(ClassGroup = (Developer), meta = (BlueprintSpawnableComponent, DisplayName = "Dev Menu"))
class RUNTIMEDEVMENU_API UDevMenuComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDevMenuComponent();

	/** Menu rows, top to bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dev Menu", meta = (TitleProperty = "{Label} {Binding}"))
	TArray<FDevMenuEntry> Entries;

	// ---------------------------------------------------------------- Input

	/** Opens and closes the menu. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input")
	TArray<FKey> ToggleKeys;

	/** Runs the selected row. Taken away from gameplay only while the menu is open. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input")
	TArray<FKey> ConfirmKeys;

	/** Move the selection with buttons. Taken away from gameplay only while the menu is open. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input")
	TArray<FKey> UpKeys;

	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input")
	TArray<FKey> DownKeys;

	/** Move the selection with the right stick. While the menu is open the stick no longer turns the camera. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input")
	bool bUseRightStick = true;

	/** Turn on if pushing the stick up moves the selection down (some projects or pads invert the right stick's Y). */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Input", meta = (EditCondition = "bUseRightStick"))
	bool bInvertStickY = false;

	/** Priority of the menu's Input Mapping Contexts: keep it above your gameplay contexts. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu|Input")
	int32 InputPriority = 10000;

	/** How far the stick must be pushed to move the selection. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu|Input", meta = (ClampMin = "0.1", ClampMax = "0.95"))
	float StickThreshold = 0.5f;

	/** Seconds before a held direction starts repeating. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu|Input", meta = (ClampMin = "0.05"))
	float RepeatDelay = 0.35f;

	/** Seconds between repeats while a direction is held. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu|Input", meta = (ClampMin = "0.02"))
	float RepeatInterval = 0.07f;

	// ---------------------------------------------------------------- Appearance

	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance")
	FString Title = TEXT("Dev Menu");

	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance")
	EDevMenuAnchor Anchor = EDevMenuAnchor::TopRight;

	/** Distance from the anchored screen corner, in Slate units. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance")
	FVector2D ScreenOffset = FVector2D(48.f, 48.f);

	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance", meta = (ClampMin = "8", ClampMax = "40"))
	float FontSize = 13.f;

	/** Rows visible at once. Longer lists scroll. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance", meta = (ClampMin = "3"))
	int32 MaxVisibleRows = 18;

	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance", meta = (ClampMin = "100"))
	float MinWidth = 320.f;

	/** Cursor, selected row and PAUSED badge. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance")
	FLinearColor AccentColor;

	/** Help line at the bottom of the menu. Leave empty to hide it. */
	UPROPERTY(EditAnywhere, Category = "Dev Menu|Appearance")
	FString FooterHint = TEXT("RS move   A select   Select close");

	// ---------------------------------------------------------------- Advanced

	/** The menu does nothing in Shipping builds unless this is on. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu")
	bool bEnableInShipping = false;

	/** Also list engine members (CharacterMovement, Actor flags...) in the Binding dropdown. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dev Menu")
	bool bListEngineMembers = false;

	// ---------------------------------------------------------------- Blueprint / C++ API

	UFUNCTION(BlueprintCallable, Category = "Dev Menu")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category = "Dev Menu")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category = "Dev Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintPure, Category = "Dev Menu")
	bool IsMenuOpen() const { return bOpen; }

	/** Adds a row that toggles the bool called BoolName on Target. */
	UFUNCTION(BlueprintCallable, Category = "Dev Menu|Rows")
	void AddToggle(const FString& Label, UObject* Target, FName BoolName);

	/** Adds a row that calls FunctionName (no parameters) on Target. */
	UFUNCTION(BlueprintCallable, Category = "Dev Menu|Rows")
	void AddFunction(const FString& Label, UObject* Target, FName FunctionName);

	/** Adds a row that runs a Blueprint event: drag from Action and pick "Create Event". */
	UFUNCTION(BlueprintCallable, Category = "Dev Menu|Rows")
	void AddAction(const FString& Label, FDevMenuAction Action);

	/** Adds a header line that can't be selected. */
	UFUNCTION(BlueprintCallable, Category = "Dev Menu|Rows")
	void AddSeparator(const FString& Label);

	/** Removes every row added at runtime. Rows from the Details panel stay. */
	UFUNCTION(BlueprintCallable, Category = "Dev Menu|Rows")
	void ClearRuntimeRows();

	/**
	 * C++ only. A row that runs any code. Pass GetState to show ON/OFF next to it.
	 *   DevMenu->AddLambda(TEXT("God mode"), [this]{ bGod = !bGod; }, [this]{ return bGod; });
	 */
	void AddLambda(const FString& Label, TFunction<void()> Execute, TFunction<bool()> GetState = nullptr);

	/** Options of the Binding dropdown. Editor only, called by the Details panel. */
	UFUNCTION()
	TArray<FString> GetBindableMembers() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class SDevMenu;

	/** A ready-to-run row. Every kind of row becomes one of these. */
	struct FRow
	{
		FString Label;
		bool bSeparator = false;
		bool bBroken = false;                       // binding not found: shown in red
		TFunction<void()> Execute;
		TFunction<TOptional<bool>()> GetState;      // set only for toggles
		double FlashUntil = 0.0;                    // brief highlight after running
	};

	bool IsAllowed() const;
	APlayerController* FindLocalPlayerController() const;
	void CreateInputObjects();
	void Bind(APlayerController* PC);
	void Unbind();
	void KeepContextsApplied();
	bool IsAnyKeyDown(const TArray<FKey>& Keys) const;
	void UpdateNavigation();
	void MoveSelection(int32 Direction);
	void EnsureValidSelection();
	void ClampScroll();
	void ExecuteSelected();

	void ResolveEditorEntries();
	UObject* ResolveObject(const FString& Name) const;
	FRow MakeToggleRow(const FString& Label, UObject* Target, FName BoolName) const;
	FRow MakeFunctionRow(const FString& Label, UObject* Target, FName FunctionName) const;
	FRow MakePauseRow(const FString& Label) const;
	static FRow MakeSeparatorRow(const FString& Label);

	TArray<FRow> Rows;
	int32 EditorRowCount = 0;
	int32 Selected = 0;
	int32 ScrollOffset = 0;
	bool bOpen = false;

	bool bWarnedNoEnhancedInput = false;
	bool bConfirmLocked = false;
	int32 HeldDirection = 0;
	double NextRepeatTime = 0.0;

	UPROPERTY(Transient) TObjectPtr<UInputAction> ToggleAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ConfirmAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> DownAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> StickAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> StickBlockAction;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> ToggleContext;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MenuContext;
	UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> MenuInput;

	TWeakObjectPtr<APlayerController> BoundController;
	TWeakObjectPtr<ULocalPlayer> BoundPlayer;
	TSharedPtr<SDevMenu> Widget;
};