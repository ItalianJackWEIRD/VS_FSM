// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "SlideState.generated.h"

class URollDataAsset;

/**
 * Slide durante Sprint
 */
UCLASS()
class ULS_API USlideState : public UPlayerBaseState
{
	GENERATED_BODY()

public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;

protected:
	void StartSlide(const URollDataAsset* SlideData);

	float Elapsed = 0.f;
	float Duration = 0.f;
	bool bSlidePlaying = false;
	
	virtual bool WantsCrouchedCapsule() const override { return true; }
};