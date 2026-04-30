// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;
	
	virtual EOrientationDirection GetOrientationDirection();
	
public:
	virtual void TickState(float DeltaTime) override;
	
	
	
	
};
