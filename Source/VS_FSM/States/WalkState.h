// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	
public:
	virtual void TickState(float DeltaTime) override;
	
	
};
