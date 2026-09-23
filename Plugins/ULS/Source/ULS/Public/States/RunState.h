// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/LocomotionState.h"
#include "RunState.generated.h"

/**
 * 
 */
UCLASS()
class ULS_API URunState : public ULocomotionState
{
	GENERATED_BODY()
	
protected:
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
public:
	virtual void TickState(float DeltaTime) override;
	
};
