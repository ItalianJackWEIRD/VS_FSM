// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "LandState.generated.h"

/**
 * Atterraggio pesante: a terra, ma senza la logica della locomotion
 * (niente pivot, turn in place, cambi di gait o di stance).
 * Il trascinamento lo fa il DA. Esce in Idle dopo HeavyDuration, o in InAir se ricadi (lo gestisce il base).
 */
UCLASS()
class ULS_API ULandState : public UPlayerBaseState
{
	GENERATED_BODY()

public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void TickState(float DeltaTime) override;

protected:
	float Elapsed = 0.f;
	float Duration = 0.f;
};