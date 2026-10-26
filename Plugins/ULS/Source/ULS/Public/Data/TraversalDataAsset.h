// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionDataAsset.h"
#include "TraversalDataAsset.generated.h"

/**
 * DA della TraversalState (hurdle, vault, mantle).
 * Mentre suona il montaggio il CMC è in Flying e lo muove il root motion: velocità, frenate e rotazione
 * ereditate non contano. Contano solo nel frame tra la fine del montaggio e l'uscita dallo stato.
 */
UCLASS()
class ULS_API UTraversalDataAsset : public ULocomotionDataAsset
{
	GENERATED_BODY()

public:
	/** Watchdog: dopo questo tempo lo stato esce comunque, anche se il montaggio non ha avvisato la fine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Traversal", meta=(ClampMin="0.5"))
	float MaxDuration = 4.f;
};