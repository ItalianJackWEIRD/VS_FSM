// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/LocoFSMAnimInstance.h"
#include "CustomComponents/LocomotionStateComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULocoFSMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnsureLocoComp();
}

void ULocoFSMAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	PullFromComponent();
}

bool ULocoFSMAnimInstance::EnsureLocoComp()
{
	if (LocoComp) return true;

	if (const AActor* Owner = GetOwningActor())
		LocoComp = Owner->FindComponentByClass<ULocomotionStateComponent>();

	// Nella preview dell'editor il componente non esiste: normale, niente log.
	if (!LocoComp && !bWarnedMissingComp && GetWorld() && GetWorld()->IsGameWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("ULocoFSMAnimInstance: LocomotionStateComponent non trovato sull'owner."));
		bWarnedMissingComp = true;
	}
	return LocoComp != nullptr;
}

/**
 * Game thread, una volta per frame, prima che il grafo venga valutato.
 * Da qui in poi il grafo legge membri locali: le letture sono thread-safe
 * per costruzione, senza dipendere da Property Access.
 */
void ULocoFSMAnimInstance::PullFromComponent()
{
	if (!EnsureLocoComp()) return;

	CharacterMovement           = LocoComp->CharacterMovement;
	StanceMode                  = LocoComp->StanceMode;

	Velocity                    = LocoComp->Velocity;
	VelocityXY                  = LocoComp->VelocityXY;
	bShouldMove                 = LocoComp->bShouldMove;
	bIsCrouched                 = LocoComp->bIsCrouched;
	MovementGait                = LocoComp->MovementGait;
	bIsAiming                   = LocoComp->bIsAiming;

	OrientationDirection        = LocoComp->OrientationDirection;
	OrientationAngle            = LocoComp->OrientationAngle;
	SmoothedDir                 = LocoComp->SmoothedDir;
	Fwd                         = LocoComp->Fwd;
	Bwd                         = LocoComp->Bwd;
	Left                        = LocoComp->Left;
	Right                       = LocoComp->Right;

	LeanAngle                   = LocoComp->LeanAngle;
	LeanStateIndex              = LocoComp->LeanStateIndex;
	bLeftShoulderLocomotion     = LocoComp->bLeftShoulderLocomotion;

	PivotAnim                   = LocoComp->PivotAnim;
	PivotStartTime              = LocoComp->PivotStartTime;
	PivotEndTimeRemaining       = LocoComp->PivotEndTimeRemaining;

	bMovStopCrouched            = LocoComp->bMovStopCrouched;
	bMovStopJogging             = LocoComp->bMovStopJogging;
	bTransitionRunInJog         = LocoComp->bTransitionRunInJog;
	PlayRate                    = LocoComp->PlayRate;

	RootYawOffset               = LocoComp->RootYawOffset;
	LastRootYawOffset           = LocoComp->LastRootYawOffset;
	bShouldTurnLeft             = LocoComp->bShouldTurnLeft;
	bShouldTurnRight            = LocoComp->bShouldTurnRight;
	TurnThreshold               = LocoComp->TurnThreshold;
	TurnAnimElapsedTime         = LocoComp->TurnAnimElapsedTime;
	FinalTurnAnim               = LocoComp->FinalTurnAnim;
	TurnYawCurveName            = LocoComp->TurnYawCurveName;
	RemainingTurnYawCurveName   = LocoComp->RemainingTurnYawCurveName;

	FinalIdleAnim               = LocoComp->FinalIdleAnim;
	FinalIdleBreakAnim          = LocoComp->FinalIdleBreakAnim;
	FinalIdleRecenterAnim       = LocoComp->FinalIdleRecenterAnim;
	FinalStanceTransitionAnim   = LocoComp->FinalStanceTransitionAnim;

	bUseSeparateBrakingFriction = LocoComp->bUseSeparateBrakingFriction;
	BrakingFriction             = LocoComp->BrakingFriction;
	GroundFriction              = LocoComp->GroundFriction;
	BrakingFrictionFactor       = LocoComp->BrakingFrictionFactor;
	BrakingDecelerationWalking  = LocoComp->BrakingDecelerationWalking;
	MinDistanceToDistanceMatch  = LocoComp->MinDistanceToDistanceMatch;
}

bool ULocoFSMAnimInstance::ShouldIdleBreak()
{
	return LocoComp && LocoComp->ShouldIdleBreak();
}

bool ULocoFSMAnimInstance::ShouldStanceTransition()
{
	return LocoComp && LocoComp->ShouldStanceTransition();
}

bool ULocoFSMAnimInstance::ShouldMovWalkJogStanceTransition()
{
	return LocoComp && LocoComp->ShouldMovWalkJogStanceTransition();
}

void ULocoFSMAnimInstance::AnimNotify_ResetStanceTransition()
{
	if (EnsureLocoComp()) LocoComp->ResetStanceTransition();
}