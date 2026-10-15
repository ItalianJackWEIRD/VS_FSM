// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "RollState.generated.h"

class URollDataAsset;

/**
 * Capriola d'atterraggio: un montaggio con root motion nello Slot dell'host, sopra entrambi i backend.
 * L'input non conta finché dura. Esce in Idle dopo RollDuration, o in InAir se ricadi (lo gestisce il base).
 */
UCLASS()
class ULS_API URollState : public UPlayerBaseState
{
	GENERATED_BODY()

public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;

protected:
	void StartRoll(const URollDataAsset* RollData);

	float Elapsed = 0.f;
	float Duration = 0.f;
	bool bRollPlaying = false;
};