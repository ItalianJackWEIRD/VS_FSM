// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "Types/LocomotionTypes.h"
#include "States/PlayerBaseState.h"
#include "LocomotionState.generated.h"

/**
 * 
 */
UCLASS()
class ULS_API ULocomotionState : public UPlayerBaseState
{
	GENERATED_BODY()

protected:
	FFloatSpringState SpringState;
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
	virtual void OnJump() override { OnTraversal(); }				// Like this to make it easier to add Jump logic
	virtual void OnTraversal() override { TryEnterTraversal(); }
	
	void RequestStanceTransition(const FString& StateKey);
	bool ShouldRecenterIdle() const;
	bool IsDiagonalRight() const;
	
	void UpdateShoulderTest();
	
	bool IsLeftFootBack() const ;
	const FPivotClip* ResolvePivotClip(EOrientationDirection Target) const;
	void CheckPivot();
	
	void SetBrakingForStanceTransition();
	
	FVector CalculateRelativeAccelerationAmount(const FVector& VelocityAcceleration) const;
	
	// Lean: il base dà 0, la locomotion a terra dà il valore vero.
	virtual float CalculateLeanFSM(float YawRate) const override;
	virtual float CalculateLeanMM(const FVector& Accel) const override;
	
public:
	virtual void TickState(float DeltaTime) override;
	
	virtual void ApplyMovementParameters() override;
};