// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/LocomotionState.h"
#include "AimState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UAimState : public ULocomotionState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
	UPROPERTY(EditDefaultsOnly, Category="Camera Data")
	TObjectPtr<UCameraModeDataAsset> AimCameraData;
	UPROPERTY(EditDefaultsOnly, Category="Camera Data")
	TObjectPtr<UCameraModeDataAsset> AimCrouchCameraData;
	
	UPROPERTY(EditDefaultsOnly, Category="Aim|Convergence", meta=(ClampMin="100.0"))
	float ConvergenceDistance = 1000.f;	// cm — 10m

	void PushYawCorrection() const;
	
	UPROPERTY(EditDefaultsOnly, Category="Aim|Pitch", meta=(ClampMin="10.0", ClampMax="90.0"))
	float SweepPitchRange = 90.f;
	
public:
	virtual void TickState(float DeltaTime) override;
	
};
