// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CustomComponents/LocomotionTypes.h"
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
	
	UPROPERTY(EditDefaultsOnly)
	float RotationRate;
	
	UPROPERTY(EditDefaultsOnly)
	int StateIndex;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Braking",
	meta=(ToolTip="Braking applicato durante la transizione di gait. 0 = usa BrakingDeceleration normale."))
	float GaitTransitionBraking = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Directional Speed",
	meta=(ClampMin="0.1", ClampMax="1.0"))
	float LateralSpeedScale = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Directional Speed",
		meta=(ClampMin="0.1", ClampMax="1.0"))
	float BackwardSpeedScale = 0.5f;
	
	/**
	* Set di Animazioni per Pivot, lasciare null se lo stato non comprende Pivotaggio.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pivot")
	FPivotDirections PivotSet;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pivot", meta=(ClampMin="-1", ClampMax="0"))
	float PivotDotThreshold = -0.65f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pivot", meta=(ClampMin="0"))
	float MinSpeedForPivot = 150.f;
	
	/**
	* Cono Forward. INVARIANTE: DA_Run >= DA_Jog e DA_Walk, altrimenti chattering Run <-> Jog
	* (si entra in Run col cono dello stato precedente, si esce con quello di Run).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Orientation", meta=(ClampMin="0", ClampMax="90"))
	float ForwardHalfAngle = 60.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Orientation", meta=(ClampMin="0", ClampMax="90"))
	float BackwardHalfAngle = 60.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Orientation", meta=(ClampMin="0", ClampMax="90"), meta=(Toolip="+ basso -> + ampio il cono per poter effettuare l'animazione. Si restringe il cono False."))
	float MinDistantFromAxisToRecenter = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Orientation", meta=(ToolTip="Più alto = più reattivo"))
	float OrientationInterpSpeed = 10.f; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Orientation", meta=(ToolTip="Soglia velocity affidabile, non KINDA_SMALL"))
	float MinSpeedForOrientation = 10.f; 

};
