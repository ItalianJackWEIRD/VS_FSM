// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StateBase.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class FSM_API UStateBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State")
	bool bCanTickState = false;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State")
	bool bCanRepeatState = false; // Whether the state can be re-entered while it's already active. If false, trying to enter the state again will be ignored until it has been exited.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State")
	FName StateDisplayName; //Debugging

public:
	virtual void OnEnterState(AActor* StateOwner);
	virtual void OnExitState();
	virtual void TickState(float DeltaTime);
};
