// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "PlayerBaseState.h"
#include "IdleState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UIdleState : public UPlayerBaseState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	
	
public:
	virtual void TickState(float DeltaTime) override;
	virtual void OnEnterState(AActor* StateOwner) override;
	
private:
	float PreviousActorYaw;
	FFloatSpringState SpringState;
};
