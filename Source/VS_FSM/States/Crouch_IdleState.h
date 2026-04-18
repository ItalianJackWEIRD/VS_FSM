// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "PlayerBaseState.h"
#include "Crouch_IdleState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UCrouch_IdleState : public UPlayerBaseState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	
	void SelectTurnAnim();
	
public:
	virtual void TickState(float DeltaTime) override;
	virtual void OnEnterState(AActor* StateOwner) override;
	
private:
	FFloatSpringState SpringState;
	float PreviousActorYaw;
	
};
