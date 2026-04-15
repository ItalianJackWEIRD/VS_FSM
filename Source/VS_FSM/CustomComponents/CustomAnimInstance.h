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

USTRUCT(BlueprintType)
struct FSt_TurnAnims
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* TurnRight90;
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* TurnLeft90;
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
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString RemainingTurnYawCurveName = FString(TEXT("RemainingTurnYaw"));
	
	ERootYawMode RootYawMode = ERootYawMode::Accumulate;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FSt_TurnAnims TurnAnimsStanding;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FSt_TurnAnims TurnAnimsCrouching;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	UAnimSequence* FinalTurnAnim = nullptr;
	UPROPERTY(BlueprintReadWrite)
	float TurnAnimElapsedTime = 0.f;
};


