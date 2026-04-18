// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerBaseState.h"
#include "Crouch_WalkState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UCrouch_WalkState : public UPlayerBaseState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	
public:
	virtual void TickState(float DeltaTime) override;
	
};
