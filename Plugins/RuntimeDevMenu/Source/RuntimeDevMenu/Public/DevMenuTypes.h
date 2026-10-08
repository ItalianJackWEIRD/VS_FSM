// Copyright (c) 2026 Andrea. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DevMenuTypes.generated.h"

/** What a menu row does. */
UENUM(BlueprintType)
enum class EDevMenuEntryType : uint8
{
	/** Toggles a bool or calls a function, picked from the Binding dropdown. */
	Binding    UMETA(DisplayName = "Bool / Function"),
	/** Pauses and resumes the game. */
	PauseGame  UMETA(DisplayName = "Pause Game"),
	/** A header line that can't be selected. */
	Separator  UMETA(DisplayName = "Separator")
};

/** Screen corner the menu sticks to. */
UENUM(BlueprintType)
enum class EDevMenuAnchor : uint8
{
	TopLeft,
	TopRight
};

/** One row of the menu, filled in the Details panel. */
USTRUCT(BlueprintType)
struct FDevMenuEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dev Menu")
	EDevMenuEntryType Type = EDevMenuEntryType::Binding;

	/** Text shown in the menu. Leave empty to use the name of the bool or function. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dev Menu")
	FString Label;

	/**
	 * Bool to toggle or function to call. "Self" is the actor that owns the menu,
	 * the other names are its components. Functions end with "()" and take no parameters.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dev Menu",
		meta = (GetOptions = "GetBindableMembers", EditCondition = "Type == EDevMenuEntryType::Binding", EditConditionHides))
	FString Binding;
};

/** Blueprint action for AddAction: in the graph, drag from the pin and pick "Create Event". */
DECLARE_DYNAMIC_DELEGATE(FDevMenuAction);
