// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "PipeState.generated.h"

class UPipeDataAsset;
class APipeActor;

/**
 * 
 */
UCLASS()
class ULS_API UPipeState : public UPlayerBaseState
{
	GENERATED_BODY()

public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;

protected:
	virtual void OnCrouch() override;

	UPROPERTY()
	APipeActor* Pipe = nullptr;
	UPROPERTY()
	const UPipeDataAsset* PipeDataAsset = nullptr;

	bool bExitCrouched = false;

	// Pipe RunTime State
	EPipePhase Phase = EPipePhase::Entering;
	float PhaseTime = 0.0f;
	FTransform EnterGrip = FTransform::Identity;
	FVector SnapDeltaLoc = FVector::ZeroVector;
	float SnapDeltaYaw = 0.0f;
	float PrevAlpha = 0.0f; // quanto spostamento ho fatto
	UPROPERTY()
	UAnimMontage* CurrentMontage = nullptr;
	
	bool bRightHand = false;
	bool bGoingUp = false;
	int32 StepCount = 0;
	float Step = 0.f;
	float EntryZ = 0.f;

	void PlayMontage(UAnimMontage* AnimMontage);
	bool IsAtEnd(UAnimMontage* AnimMontage);
	void SetRootZ(float Z);
	
	float RootZ() const;
	void ExitToGround();
	
	float RootMotionZ(UAnimMontage* AnimMontage) const;
	void StartStroke(bool bUp);
	
	// Breathing Idle
	float BreathTime = 0.f;
	float BreathValue = 0.f;						// 0 = fermo, 1 = massimo dell'inspirazione
	FVector MeshBaseScale = FVector::OneVector;		// la scala della mesh prima di toccarla
	void UpdateBreath(float DeltaTime);
};
