// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VSCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UVSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	virtual float GetMaxSpeed() const override;

	/** Moltiplicatori direzionali. Scritti dagli stati in OnEnterState. */
	float LateralScale  = 1.f;
	float BackwardScale = 1.f;
};
