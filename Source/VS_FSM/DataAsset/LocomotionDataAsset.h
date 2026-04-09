// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LocomotionDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API ULocomotionDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public: 
	UPROPERTY(EditDefaultsOnly)
	float MovementSpeed;
	
	UPROPERTY(EditDefaultsOnly)
	float MaxAcceleration;
	
	UPROPERTY(EditDefaultsOnly)
	float BrakingDeceleration;
	
	UPROPERTY(EditDefaultsOnly)
	float BrakingFrictionFactor;
	
	UPROPERTY(EditDefaultsOnly)
	float BrakingFriction;
	
	UPROPERTY(EditDefaultsOnly)
	bool bUseSeparateBrakingFriction;
};
