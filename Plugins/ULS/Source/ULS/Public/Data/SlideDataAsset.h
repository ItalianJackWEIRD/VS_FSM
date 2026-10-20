// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RollDataAsset.h"
#include "SlideDataAsset.generated.h"

class UAnimMontage;

/**
 * DA della Slide State
 */
UCLASS()
class ULS_API USlideDataAsset : public URollDataAsset
{
	GENERATED_BODY()

public:
	/** Moltiplicatore del root motion all'inizio dello slide: rinforza la spinta d'ingresso della clip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slide", meta=(ClampMin="1"))
	float PushBoostScale = 1.5f;

	/** In quanti secondi il moltiplicatore torna a 1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slide", meta=(ClampMin="0"))
	float PushBoostDuration = 0.4f;
};