// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
#include "CustomComponents/ShootingSystem/WeaponDataAsset.h"
#include "VS_FSMCharacter.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

class UBlendSpace;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class VS_FSM_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	
	/*--- This is needed for states and transition in ABP that makes the editor crash, we just
	 * give them this bool for transition rules, and we let them stay sleep forever ---*/
	UPROPERTY(BlueprintReadOnly)
	bool bAlwaysFalse = false;
	
	
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
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General ABP Settings")
	float PlayRate = 1.f;
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInIdle = false;
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInMovStop = false;
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInRunStop = false;
	
	//Idle Break
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	UAnimSequence* FinalIdleBreakAnim = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Idle")
	FFour_Anims IdleBreakAnims;
	bool bShouldIdleBreak = false;
	UPROPERTY(BlueprintReadWrite, Category="Idle")
	bool bIsIdleBreak = false;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldIdleBreak();
	
	//Idle Recentering
	UPROPERTY(BlueprintReadWrite, Category="Idle")
	bool bShouldRecenterIdle = false;
	UPROPERTY(EditDefaultsOnly, Category="Idle")
	FTwo_Anims IdleRecenterAnims;
	UPROPERTY(EditDefaultsOnly, Category="Idle")
    FTwo_Anims IdleCrouchRecenterAnims;
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	UAnimSequence* FinalIdleRecenterAnim = nullptr;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Turn In Place")
	float TurnThreshold;
	
	///!!!ç Potrebbero essere entrambe inutili
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	FString RemainingTurnYawCurveName = FString(TEXT("RemainingTurnYaw"));
	
	// Set Animations	-	01 means Stand
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims TurnAnimsStanding;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims TurnAnimsCrouching;
	
	UPROPERTY(EditDefaultsOnly, Category="Idle")
	FTwo_Anims IdleAnims;
	
	UPROPERTY(EditDefaultsOnly, Category="Turn In Place")
	FTwo_Anims StanceTransitionAnims;
	
	
	// Anim Reference (potrei togliere editdefaultsonly )
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Turn In Place")
	UAnimSequence* FinalTurnAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalIdleAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalStanceTransitionAnim = nullptr;
	
	
	UPROPERTY(BlueprintReadWrite)
	float TurnAnimElapsedTime = 0.f;
	
	
	// parametri per il sistema idle doppio Stand / Crouch
	
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouched = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bMovStopCrouched = false;
	
	bool bShouldStanceTransition = false;
	bool bIsInStanceTransition = false;
	float StanceTransitionStartTime;
	
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
	FVector SmoothedDir = FVector::ForwardVector;
	
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
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Jog")
	bool bIsRunning = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Jog")
	bool bMovStopJogging = false;
	UPROPERTY(EditDefaultsOnly, Category="Locomotion Jog", meta=(
	ToolTip="Settala a metà strada tra la tua MaxWalkSpeed di walk e quella di jog",
	ClampMin="0.0",
	ClampMax="1000.0",
	UIMin="0.0",
	UIMax="1000.0"))
	float MovStopJogSpeedThreshold = 350.f;
	
	bool bShouldWalkJogStanceTransition = false;
	bool bIsInWalkJogStanceTransition = false;
	float WalkJogTransitionStartTime = 0.f;
	
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldMovWalkJogStanceTransition();
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion Run")
	bool bTransitionRunInJog = false;
	
	// FLARE - deprecated --> will be moved into Equippables
	UPROPERTY(BlueprintReadOnly, Category="Flare")
	bool bFlare = false;
	UPROPERTY(BlueprintReadOnly, Category="Flare")
	float FlareAlpha = 0.f;
	UPROPERTY(EditDefaultsOnly, Category="Flare")
	float FlareBlendSpeed = 8.f;
	
	
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
	// + the min distance to distance match ( = 30.f , only changed on Enter and Exit of AimState) 
	UPROPERTY(BlueprintReadOnly)
	float MinDistanceToDistanceMatch = 30.f;
	
	// ---> Weapon System
	/**/
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpperBodyOn = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpper1H = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpper2H = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float Weapon1hAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float Weapon2hAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float GripAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	EWeaponGrip WeaponGrip = EWeaponGrip::OneHand;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bIsAiming = false;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay1HStand = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay1HCrouch = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay2HStand = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay2HCrouch = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UAnimSequence> EquipUnEquipAnim = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	float OverlayHeight = 1.f;
	
	UPROPERTY(BlueprintReadWrite, Category="Weapon|Overlay")
	bool bShouldEquipWeapon = false;

	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	TObjectPtr<UAnimSequence> FinalAimPose = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimAlpha = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimPitch = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimYawCorrection = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimLeanAngle = 0.f;	
	
	
	// Enemy Detection (Stance)
	float TimerEnemyPoll = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Enemy Detection")
	bool bEnemyDetected = false;
	
	
	
protected:	
	UPROPERTY(BlueprintReadOnly)
	AVS_FSMCharacter* PlayerRef = nullptr;
	
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	EStanceMode GetStanceMode() const;
	
};