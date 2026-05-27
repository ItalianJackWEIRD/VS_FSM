// Fill out your copyright notice in the Description page of Project Settings.


#include "States/LocomotionState.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULocomotionState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
}

void ULocomotionState::OnExitState()
{
	Super::OnExitState();
}

FVector ULocomotionState::GetIntendedDir()
{
	FVector Dir = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
	if (Dir.IsNearlyZero()) Dir = PlayerRef->GetPendingMovementInputVector(); // input non ancora consumato
	if (Dir.IsNearlyZero()) Dir = PlayerRef->GetVelocity();
	return Dir.GetSafeNormal2D(); // può tornare zero se davvero non c'è nulla
}

void ULocomotionState::PushOrientationDirection(FVector InSmoothedDir)
{	
	if (AnimInstance->SmoothedDir.IsNearlyZero()) return;
	
	const FVector Forward = PlayerRef->GetActorForwardVector();
	const float Dot = FVector::DotProduct(Forward, InSmoothedDir);
	const float CrossZ = FVector::CrossProduct(Forward, InSmoothedDir).Z;
	
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	
	AnimInstance->OrientationAngle = Angle;
	AnimInstance->Fwd   = FMath::UnwindDegrees(Angle);
	AnimInstance->Bwd   = FMath::UnwindDegrees(Angle - 180.f);
	AnimInstance->Left  = FMath::UnwindDegrees(Angle + 90.f);
	AnimInstance->Right = FMath::UnwindDegrees(Angle - 90.f);
	
	const float AbsAngle = FMath::Abs(Angle);
	if (AbsAngle <= StateData->ForwardHalfAngle)   AnimInstance->OrientationDirection = EOrientationDirection::Forward;
	else if (AbsAngle >= 180.f - StateData->BackwardHalfAngle)    AnimInstance->OrientationDirection = EOrientationDirection::Backward;
	else if (Angle >= 0)   AnimInstance->OrientationDirection = EOrientationDirection::Right;
	else AnimInstance->OrientationDirection = EOrientationDirection::Left;
}

void ULocomotionState::UpdateOrientationDirection(float DeltaTime)		//Also Update values of direction in ABP -> Now we take accelleration, safer
{
	const FVector Velocity = PlayerRef->GetVelocity();
	FVector TargetDir;
	
	if (Velocity.Size2D() >= StateData->MinSpeedForOrientation)	
	{
		TargetDir = Velocity.GetSafeNormal2D();
	}
	else
	{
		const FVector Accel = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
		if (!Accel.IsNearlyZero())
			TargetDir = Accel.GetSafeNormal2D();
		else
			return; // quasi a 0, scarta tutto
	}
	
	AnimInstance->SmoothedDir = FMath::VInterpTo(AnimInstance->SmoothedDir, TargetDir, DeltaTime, StateData->OrientationInterpSpeed).GetSafeNormal2D();
	PushOrientationDirection(AnimInstance->SmoothedDir);
}

void ULocomotionState::RequestStanceTransition(const FString& StateKey)
{
	if (AnimInstance->bIsInStanceTransition) return; // sto già transizionando (reset via notify)
	
	if (AnimInstance->bAnimGraphInIdle || AnimInstance->bAnimGraphInMovStop)
	{
		AnimInstance->bShouldStanceTransition   = true;  // trigger consumato dall'AnimGraph
		AnimInstance->bIsInStanceTransition      = true;  // guardia
		AnimInstance->StanceTransitionStartTime  = PlayerRef->GetWorld()->GetTimeSeconds(); // timbro watchdog
	}
	
	PlayerRef->StateManager->SwitchStateByKey(StateKey);
}

void ULocomotionState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	const bool bShouldMoveNow = !PlayerRef->IsMovementInputZero();
	// Edge true→false = we are entering in Mov Stop → freeze gait for Anim Stop
	if (AnimInstance->bShouldMove && !bShouldMoveNow)
	{
		AnimInstance->bMovStopJogging = AnimInstance->bIsJogging;
		AnimInstance->bMovStopCrouched = AnimInstance->bIsCrouched;
	}	
	
	// così Movement Start legge valori freschi anche se il C++ è ancora in Idle
	if (!AnimInstance->bShouldMove && bShouldMoveNow)
	{
		AnimInstance->SmoothedDir = GetIntendedDir();
		PushOrientationDirection(AnimInstance->SmoothedDir);
	}
	
	AnimInstance->bShouldMove = bShouldMoveNow;
	
	//Fallback for Jog->Walk (bug - resolved with this) -> might cause bugs in idle
	if (AnimInstance->bIsInWalkJogStanceTransition)
	{
		const float Elapsed = PlayerRef->GetWorld()->GetTimeSeconds() - AnimInstance->WalkJogTransitionStartTime;
		if (Elapsed > 8.f) AnimInstance->bIsInWalkJogStanceTransition = false;
	}
	if (AnimInstance->bIsInStanceTransition)
	{
		const float Elapsed = PlayerRef->GetWorld()->GetTimeSeconds() - AnimInstance->StanceTransitionStartTime;
		if (Elapsed > 3.f) AnimInstance->bIsInStanceTransition = false; 
	}
}
