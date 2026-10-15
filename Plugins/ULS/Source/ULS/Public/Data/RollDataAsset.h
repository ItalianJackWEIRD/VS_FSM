// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionDataAsset.h"
#include "RollDataAsset.generated.h"

class UAnimMontage;

/**
 * DA della RollState (capriola d'atterraggio).
 * Il montaggio ha il root motion: è lui a muovere la capsula, i campi ereditati del CMC non contano finché suona.
 */
UCLASS()
class ULS_API URollDataAsset : public ULocomotionDataAsset
{
	GENERATED_BODY()

public:
	/** Montaggio della capriola, nello Slot azioni dell'host. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roll")
	TObjectPtr<UAnimMontage> RollMontage = nullptr;

	/** Da dove parte il montaggio: il frame di contatto. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roll", meta=(ClampMin="0"))
	float RollStartTime = 0.97f;

	/** Quanto dura la capriola dallo start: poi lo stato torna in Idle e il montaggio sfuma. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roll", meta=(ClampMin="0"))
	float RollDuration = 0.92f;

	/** Blend out del montaggio all'uscita dallo stato. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roll", meta=(ClampMin="0"))
	float RollBlendOut = 0.2f;
};