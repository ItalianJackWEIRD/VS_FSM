// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "CustomComponents/LocomotionTypes.h"
#include "PlayerBaseState.h"
#include "LocomotionState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API ULocomotionState : public UPlayerBaseState
{
	GENERATED_BODY()

protected:
	FVector SmoothedDir = FVector::ForwardVector;
	FFloatSpringState SpringState;
	float PreviousActorYaw = 0.0f;
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
	FVector GetIntendedDir();
	void PushOrientationDirection(FVector InSmoothedDir);
	virtual void UpdateOrientationDirection(float DeltaTime);
	
public:
	virtual void TickState(float DeltaTime) override;
	
};
