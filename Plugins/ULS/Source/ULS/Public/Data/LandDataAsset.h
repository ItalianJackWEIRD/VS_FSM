// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionDataAsset.h"
#include "LandDataAsset.generated.h"

/**
 * DA della LandState (atterraggio pesante).
 * I campi ereditati configurano il CMC della ripresa: velocità bassissima, frenata alta, rotazione lenta.
 */
UCLASS()
class ULS_API ULandDataAsset : public ULocomotionDataAsset
{
	GENERATED_BODY()

public:
	/** Quanto dura la ripresa dopo un atterraggio pesante, prima di tornare in Idle. (0.5 Alert) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Land", meta=(ClampMin="0"))
	float HeavyDuration = 0.6f;
};