// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionDataAsset.h"
#include "PipeDataAsset.generated.h"

class UAnimMontage;

/**
 * DA della PipeState.
 */
UCLASS()
class ULS_API UPipeDataAsset : public ULocomotionDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	float GripDistance = 36.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	float EnterSnapTime = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	float InputThreshold = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	float DetachPush = 0.f;
	
	/** Enter/Exit Anims */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* EnterBottom = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* ExitTop = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* EnterTop = nullptr;
	
	/** Moving Anims */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* ClimbUpLeft = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* ClimbUpRight = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* ClimbDownLeft = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pipe")
	UAnimMontage* ClimbDownRight = nullptr;
	
};