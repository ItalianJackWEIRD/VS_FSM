// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	float RootYawOffset;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnRight;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnLeft;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
};
