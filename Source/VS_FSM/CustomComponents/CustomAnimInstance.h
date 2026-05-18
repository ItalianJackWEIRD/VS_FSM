// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class VS_FSM_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	//Reference generali
	UPROPERTY(BlueprintReadOnly)
	UCharacterMovementComponent* CharacterMovement = nullptr;
	
	/* 
	 * Variable that returns an index based on current state for Leaning, 0 CrouchWalk, 1 Idle/Walk, 2 Jog, 3 Run ---> Use it in BlendSpace
	 * It is changed everytime in OnEnterState; it takes the int from the State Data we created for each State. 
	 */
	UPROPERTY(BlueprintReadOnly)
	int StateIndex = 0; //Idle
	
	
	UPROPERTY(BlueprintReadOnly)
	float RootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float LastRootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnRight = false;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnLeft = false;
	
	ERootYawMode RootYawMode = ERootYawMode::Accumulate;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
	
	// Potrebbero essere entrambe inutili
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString RemainingTurnYawCurveName = FString(TEXT("RemainingTurnYaw"));
	
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
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetMovWalkJogChange();
	
	// LOCOMOTION
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Fwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Bwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Left = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Right = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector VelocityXY = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float OrientationAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LeanAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Jog")
	bool bIsJogging = false;
	bool bShouldWalkJogStanceTransition = false;
	bool bIsInWalkJogStanceTransition = false;
	
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldMovWalkJogStanceTransition();
	
	void RefreshDataAsset();
	
	// Cache per Distance Matching
	UPROPERTY(BlueprintReadOnly)
	bool bUseSeparateBrakingFriction = false;
	UPROPERTY(BlueprintReadOnly)
	float BrakingFriction = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float GroundFriction = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float BrakingFrictionFactor = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float BrakingDecelerationWalking = 0.f;
	
};


