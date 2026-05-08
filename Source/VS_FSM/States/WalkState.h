// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "CustomComponents/LocomotionTypes.h"
#include "PlayerBaseState.h"
#include "WalkState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UWalkState : public UPlayerBaseState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	virtual void OnToggleJog() override;
	
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;
	
	virtual void UpdateOrientationDirection();
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
public:
	virtual void TickState(float DeltaTime) override;
	
private:
	void UpdateAnimationParameters(float DeltaTime);
	FFloatSpringState SpringState;
	float PreviousActorYaw = 0.0f;
	
};
