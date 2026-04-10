// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

UENUM()
enum class ERootYawMode : uint8
{
	Accumulate,
	BlendOut
};
/**
 * 
 */
UCLASS()
class VS_FSM_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	float RootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float LastRootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnRight = false;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnLeft = false;
	
	ERootYawMode RootYawMode = ERootYawMode::Accumulate;
	//FFloatSpringState SpringState;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
};


