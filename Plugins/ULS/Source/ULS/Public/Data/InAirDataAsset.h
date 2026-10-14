// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionDataAsset.h"
#include "InAirDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class ULS_API UInAirDataAsset : public ULocomotionDataAsset
{
	GENERATED_BODY()
	
public: 
	/** Quota dell'accelerazione che arriva in aria: accelerazione laterale = MaxAcceleration * AirControl. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air", meta=(ClampMin="0", ClampMax="1"))
	 float AirControl = 0.05f;
	
	/** Il CMC di default vale 2: raddoppia l'air control quando cadi quasi da fermo. A 1 non raddoppia. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air", meta=(ClampMin="1"))
	float AirControlBoostMultiplier = 1.f;
	
	/** Sopra zero perdi slancio orizzontale mentre cadi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air", meta=(ClampMin="0"))
	float BrakingDecelerationFalling = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air", meta=(ClampMin="0"))
	float FallingLateralFriction = 0.f;
	
	// il check avviene in base alla velocità con cui facciamo landing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air|Land", meta=(ClampMin="0"))
	float HeavyLandSpeed = 600.f;
	
	// Il crouch vale come capriola se premuto entro questo tempo dall'impatto.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Air|Land", meta=(ClampMin="0"))
	float RollWindow = 0.4f;
};
