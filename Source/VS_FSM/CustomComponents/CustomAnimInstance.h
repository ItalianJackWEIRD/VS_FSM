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
struct FTwo_Anims
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* R_01 = nullptr;
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* L_02 = nullptr;
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
	
	// Potrebbero essere entrambe inutili
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString RemainingTurnYawCurveName = FString(TEXT("RemainingTurnYaw"));
	
	ERootYawMode RootYawMode = ERootYawMode::Accumulate;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
	
	// Set Animations	-	01 means Stand
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims TurnAnimsStanding;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims TurnAnimsCrouching;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims IdleAnims;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims StanceTransitionAnims;
	//
	
	// Anim Reference
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	UAnimSequence* FinalTurnAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalIdleAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalStanceTransitionAnim = nullptr;
	//
	
	UPROPERTY(BlueprintReadWrite)
	float TurnAnimElapsedTime = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	float PlayRate = 1.f;
	
	// parametri per il sistema idle doppio Stand / Crouch
	
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();
	
	bool bIsCrouched = false;
	bool bShouldStanceTransition = false;
	bool bIsInStanceTransition = false;
	
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();
	
	
};


