// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
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
	
	// Anim Reference (potrei togliere editdefaultsonly )
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	UAnimSequence* FinalTurnAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalIdleAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalStanceTransitionAnim = nullptr;
	//
	
	UPROPERTY(BlueprintReadWrite)
	float TurnAnimElapsedTime = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	float PlayRate = 1.f;
	
	// parametri per il sistema idle doppio Stand / Crouch
	
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouched = false;
	bool bShouldStanceTransition = false;
	bool bIsInStanceTransition = false;
	
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();
	
	// LOCOMOTION
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsJogging = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector VelocityXY = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float OrientationAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Fwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Bwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Left = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Right = 0.f;
	
	
};


