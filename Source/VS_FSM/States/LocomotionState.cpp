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
	
	if (AnimInstance->bAnimGraphInIdle || AnimInstance->bAnimGraphInMovStop || AnimInstance->bAnimGraphInRunStop)
	{
		AnimInstance->bShouldStanceTransition   = true;  // trigger consumato dall'AnimGraph
		AnimInstance->bIsInStanceTransition      = true;  // guardia
		AnimInstance->StanceTransitionStartTime  = PlayerRef->GetWorld()->GetTimeSeconds(); // timbro watchdog
	}
	
	PlayerRef->StateManager->SwitchStateByKey(StateKey);
}

bool ULocomotionState::ShouldRecenterIdle() const
{
	// solo gait fwd/bwd: coi coni a 65 i diagonali cadono dentro questi bucket
	const EOrientationDirection Dir = AnimInstance->OrientationDirection;
	if (Dir != EOrientationDirection::Forward && Dir != EOrientationDirection::Backward)
		return false;

	// distanza dalla cardinale fwd(0)/bwd(180) più vicina, sull'ultimo angolo "in movimento"
	const float Abs = FMath::Abs(AnimInstance->OrientationAngle);
	const float DistFromAxis = FMath::Min(Abs, 180.f - Abs);

	return DistFromAxis >= StateData->MinDistantFromAxisToRecenter;
}

bool ULocomotionState::IsDiagonalRight() const
{
	const float Angle = AnimInstance->OrientationAngle;
	const EOrientationDirection Dir = AnimInstance->OrientationDirection;
	
	if (Dir == EOrientationDirection::Forward) return Angle > 0.f; // es. +45° = forward-right
	if (Dir == EOrientationDirection::Backward) return Angle > 0.f && Angle < 180.f; // es. +135° = backward-right | -135° sarebbe backward-left
	
	return false;
}

void ULocomotionState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	const bool bShouldMoveNow = !PlayerController->IsMovementInputZero();
	// Edge true→false = we are entering in Mov Stop → freeze gait for Anim Stop -> check if recentering animation is needed
	if (AnimInstance->bShouldMove && !bShouldMoveNow)
	{
		AnimInstance->bMovStopJogging = PlayerRef->GetVelocity().Size2D() > AnimInstance->MovStopJogSpeedThreshold; // now the bool is calculated based on physics and not input.
		AnimInstance->bMovStopCrouched = AnimInstance->bIsCrouched;
		if (ShouldRecenterIdle())
		{
			AnimInstance->bShouldRecenterIdle = true;
			if (AnimInstance->OrientationDirection == EOrientationDirection::Forward)
			{
				if (IsDiagonalRight())
					if (AnimInstance->bIsCrouched)
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleCrouchRecenterAnims.L_02;
					else
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleRecenterAnims.L_02;
				else
					if (AnimInstance->bIsCrouched)
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleCrouchRecenterAnims.R_01;
					else
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleRecenterAnims.R_01;
			}
			else
			{
				if (IsDiagonalRight())
					if (AnimInstance->bIsCrouched)
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleCrouchRecenterAnims.R_01;
					else
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleRecenterAnims.R_01;
				else
					if (AnimInstance->bIsCrouched)
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleCrouchRecenterAnims.L_02;
					else
						AnimInstance->FinalIdleRecenterAnim = AnimInstance->IdleRecenterAnims.L_02;
			}
		}
		else AnimInstance->bShouldRecenterIdle = false;
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
		if (Elapsed > 3.f) AnimInstance->bIsInWalkJogStanceTransition = false;
	}
	if (AnimInstance->bIsInStanceTransition)
	{
		const float Elapsed = PlayerRef->GetWorld()->GetTimeSeconds() - AnimInstance->StanceTransitionStartTime;
		if (Elapsed > 3.f) AnimInstance->bIsInStanceTransition = false; 
	}
	
#pragma region DEBUG
	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Magenta,
	FString::Printf(TEXT("Stance: %s"), *UEnum::GetValueAsString(PlayerRef->GetStanceMode())));
#pragma endregion DEBUG
}
