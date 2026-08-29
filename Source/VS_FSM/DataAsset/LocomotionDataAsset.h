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
	
	UPROPERTY(EditDefaultsOnly)
	int StateIndex;
	
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
