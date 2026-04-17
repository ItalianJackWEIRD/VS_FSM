// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/WalkState.h"
#include "Crouch_WalkState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UCrouch_WalkState : public UPlayerBaseState
{
	GENERATED_BODY()
	
public:
	virtual void TickState(float DeltaTime) override;
	
};
