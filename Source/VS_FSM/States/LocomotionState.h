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
	FFloatSpringState SpringState;
	float PreviousActorYaw = 0.0f;
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
	FVector GetIntendedDir();
	void PushOrientationDirection(FVector InSmoothedDir);
	virtual void UpdateOrientationDirection(float DeltaTime);
	
	void RequestStanceTransition(const FString& StateKey);
	bool ShouldRecenterIdle() const;
	bool IsDiagonalRight() const;
	
	void UpdateAnimationParameters(float DeltaTime);
	
	void UpdateShoulderTest();
	
public:
	virtual void TickState(float DeltaTime) override;
};
