// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "InAirState.generated.h"

/**
 * 
 */
UCLASS()
class ULS_API UInAirState : public UPlayerBaseState
{
	GENERATED_BODY()
	
public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;
	
	virtual void ApplyMovementParameters() override;
	
protected:
	virtual void OnStartFalling() override {}
	virtual void OnLanded(const FHitResult& Hit);
	
	virtual void OnCrouch() override;
	virtual void OnJump() override { OnTraversal(); }				// Like this to make it easier to add Jump logic
	virtual void OnTraversal() override { TryEnterTraversal(); }
	
	float RollRequestTime = -1000.f;
};
