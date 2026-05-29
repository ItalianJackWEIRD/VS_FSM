// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionState.h"
#include "Kismet/KismetMathLibrary.h"
#include "PlayerBaseState.h"
#include "IdleState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UIdleState : public ULocomotionState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	
	void SelectTurnAnim();
	
public:
	virtual void TickState(float DeltaTime) override;
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
private:
	void UpdateAnimationParameters();
	
	float TimerToBreakIdle = 0.0f;
};
