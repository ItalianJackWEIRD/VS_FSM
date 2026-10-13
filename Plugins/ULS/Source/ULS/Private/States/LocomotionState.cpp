// Fill out your copyright notice in the Description page of Project Settings.


#include "States/LocomotionState.h"
#include "Data/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULocomotionState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	LocoComp->LocomotionMode = ELocomotionMode::Ground;
}

void ULocomotionState::OnExitState()
{
	Super::OnExitState();
}

void ULocomotionState::RequestStanceTransition(const FString& StateKey)
{
	if (LocoComp->bIsInStanceTransition) return; // sto già transizionando (reset via notify)
	
	if (LocoComp->bAnimGraphInIdle || LocoComp->bAnimGraphInMovStop || LocoComp->bAnimGraphInRunStop)
	{
		LocoComp->bShouldStanceTransition   = true;  // trigger consumato dall'AnimGraph
		LocoComp->bIsInStanceTransition      = true;  // guardia
		LocoComp->StanceTransitionStartTime  = PlayerRef->GetWorld()->GetTimeSeconds(); // timbro watchdog
	}
	
	StateManager->SwitchStateByKey(StateKey);
}

bool ULocomotionState::ShouldRecenterIdle() const
{
	if (!LocoComp->bEnableIdleRecenter) return false;
	
	// solo gait fwd/bwd: coi coni a 65 i diagonali cadono dentro questi bucket
	const EOrientationDirection Dir = LocoComp->OrientationDirection;
	if (Dir != EOrientationDirection::Forward && Dir != EOrientationDirection::Backward)
		return false;

	// distanza dalla cardinale fwd(0)/bwd(180) più vicina, sull'ultimo angolo "in movimento"
	const float Abs = FMath::Abs(LocoComp->OrientationAngle);
	const float DistFromAxis = FMath::Min(Abs, 180.f - Abs);

	return DistFromAxis >= StateData->MinDistantFromAxisToRecenter;
}

bool ULocomotionState::IsDiagonalRight() const
{
	const float Angle = LocoComp->OrientationAngle;
	const EOrientationDirection Dir = LocoComp->OrientationDirection;
	
	if (Dir == EOrientationDirection::Forward) return Angle > 0.f; // es. +45° = forward-right
	if (Dir == EOrientationDirection::Backward) return Angle > 0.f && Angle < 180.f; // es. +135° = backward-right | -135° sarebbe backward-left
	
	return false;
}

void ULocomotionState::UpdateShoulderTest()
{
	if (!LocoComp || !PlayerRef || !LocoComp->bEnableShoulderVariants) return;

	const UWorld* World = PlayerRef->GetWorld();
	if (!World) return;

	constexpr float Interval = 10.f;

	const int32 Slot  = FMath::FloorToInt(World->GetTimeSeconds() / Interval);
	const bool  bLeft = (Slot % 2) == 0;

	if (bLeft == LocoComp->bLeftShoulderLocomotion) return;   // edge only

	LocoComp->bLeftShoulderLocomotion = bLeft;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
		FString::Printf(TEXT("Shoulder -> %s"), bLeft ? TEXT("LEFT") : TEXT("RIGHT")));
}

/* ---> PIVOT
**/

bool ULocomotionState::IsLeftFootBack() const
{
	const USkeletalMeshComponent* Mesh = PlayerRef->GetMesh();
	if (!Mesh) return true;
	
	const FVector TravelDir = PlayerRef->GetVelocity().GetSafeNormal2D();
	const FVector L = Mesh->GetSocketLocation(TEXT("foot_l"));
	const FVector R = Mesh->GetSocketLocation(TEXT("foot_r"));
	
	return FVector::DotProduct(L-R, TravelDir) < 0.f;
}

const FPivotClip* ULocomotionState::ResolvePivotClip(EOrientationDirection Target) const
{
	const FPivotDirections& Set = StateData->PivotSet;
	const bool bLeftShoulder = LocoComp->bLeftShoulderLocomotion;
	
	const FPivotFeet* Feet = nullptr;
	
	switch (Target)
	{
		case EOrientationDirection::Forward: Feet = &Set.FromForward; break;
		case EOrientationDirection::Backward: Feet = &Set.FromBackward; break;
	case EOrientationDirection::Left: Feet = (!bLeftShoulder && Set.FromLeft.R.LFoot.Anim && Set.FromLeft.R.RFoot.Anim) ? &Set.FromLeft.R : &Set.FromLeft.L;
		break;
	case EOrientationDirection::Right: Feet = (!bLeftShoulder && Set.FromRight.R.LFoot.Anim && Set.FromRight.R.RFoot.Anim) ? &Set.FromRight.R : &Set.FromRight.L;
		break;
	}
	
	if (!Feet) return nullptr;
	
	const FPivotClip& Clip = IsLeftFootBack() ? Feet->LFoot : Feet->RFoot;
	return Clip.Anim ? &Clip : nullptr; // niente clip: lo stato non pivota.
}

void ULocomotionState::CheckPivot()
{
	if (!LocoComp || !PlayerRef || !StateData) return;
	if (!LocoComp->bEnablePivot) return;
	if (LocoComp->bShouldPivot) return; // aspetto il consumo
	
	const FVector Vel = PlayerRef->GetVelocity();
	if (Vel.Size2D() < StateData->MinSpeedForPivot) return;
	
	const FVector Accel = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
	if (Accel.IsNearlyZero()) return;
	
	if (FVector::DotProduct(Vel.GetSafeNormal2D(), Accel.GetSafeNormal2D()) > StateData->PivotDotThreshold) return;
	
	const FPivotClip* Clip = ResolvePivotClip(LocoComp->OrientationDirection);
	if (!Clip) return; // Guard, no pivot se non ha anim assgnate.
	
	LocoComp->PivotAnim = Clip->Anim;
	LocoComp->PivotStartTime = Clip->StartTime;
	LocoComp->PivotEndTimeRemaining = Clip->Anim->GetPlayLength() - Clip->EndTime;
	LocoComp->bShouldPivot = true;
}


/*
 * This function rewrites the Braking Deceleration only
 * if it's needed. For now it's needed when bIsInWalkJogStanceTransition is true.
 * That's when we are transitioning from Jog to Walk when FWD.
 * Specific piece of code for one Animation actually.
 */
void ULocomotionState::SetBrakingForStanceTransition()
{
	if (!LocoComp->bEnableDistanceMatching) return;
	
	const bool bWantsTransitionBraking =
	LocoComp->bIsInWalkJogStanceTransition
	&& StateData->GaitTransitionBraking > 0.f
	&& LocoComp->OrientationDirection == EOrientationDirection::Forward;

	const float TargetBraking = bWantsTransitionBraking
		? StateData->GaitTransitionBraking
		: StateData->BrakingDeceleration;

	UCharacterMovementComponent* CMC = PlayerRef->GetCharacterMovement();
	if (!FMath::IsNearlyEqual(CMC->BrakingDecelerationWalking, TargetBraking))
	{
		CMC->BrakingDecelerationWalking = TargetBraking;
		LocoComp->BrakingDecelerationWalking = TargetBraking;   // il Distance Matching lo legge
	}
}


FVector ULocomotionState::CalculateRelativeAccelerationAmount(const FVector& VelocityAcceleration) const
{
	const UCharacterMovementComponent* CMC = PlayerRef->GetCharacterMovement();
	const float MaxAccel   = CMC->MaxAcceleration;   // GASP legge la proprietà
	const float MaxBraking = CMC->GetMaxBrakingDeceleration();
	if (MaxAccel <= 0.f || MaxBraking <= 0.f) return FVector::ZeroVector;

	// Ramo: accelerazione da input. Valore: accelerazione misurata.
	const bool bAccelerating = FVector::DotProduct(CMC->GetCurrentAcceleration(), PlayerRef->GetVelocity()) > 0.f;
	const float MaxAmount = bAccelerating ? MaxAccel : MaxBraking;

	return PlayerRef->GetActorRotation().UnrotateVector(VelocityAcceleration.GetClampedToMaxSize(MaxAmount) / MaxAmount);
}


float ULocomotionState::CalculateLeanFSM(float YawRate) const
{
	float DirectionSign = 1.f;
	switch (LocoComp->OrientationDirection)
	{
	case EOrientationDirection::Forward: DirectionSign = 1.f; break;
	case EOrientationDirection::Backward: DirectionSign = -1.f; break;
	case EOrientationDirection::Left: DirectionSign = 1.f; break;
	case EOrientationDirection::Right: DirectionSign = -1.f; break;
	}
	
	const float RawLean = (YawRate / 4.f) * DirectionSign;
	return FMath::Clamp(RawLean, -45.f, 45.f);
}

float ULocomotionState::CalculateLeanMM(const FVector& VelocityAcceleration) const
{
	const float SpeedScale = FMath::GetMappedRangeValueClamped(FVector2f(200.f, 500.f), FVector2f(0.5f, 1.f), static_cast<float>(LocoComp->VelocityXY.Size()));
	return CalculateRelativeAccelerationAmount(VelocityAcceleration).Y * SpeedScale;
}


void ULocomotionState::TickState(float DeltaTime)
{
	const bool bPrevShouldMove = LocoComp->bShouldMove;
	Super::TickState(DeltaTime);	// --> bShouldMove
	
	const bool bShouldMoveNow = LocoComp->bShouldMove;
	
#pragma region MOVSTOP// Edge true→false = we are entering in Mov Stop → freeze gait for Anim Stop -> check if recentering animation is needed
	if (bPrevShouldMove && !bShouldMoveNow)
	{
		LocoComp->bMovStopJogging = PlayerRef->GetVelocity().Size2D() > LocoComp->MovStopJogSpeedThreshold; // now the bool is calculated based on physics and not input.
		LocoComp->bMovStopCrouched = LocoComp->bIsCrouched;
		
		if (ShouldRecenterIdle())
		{
			LocoComp->bShouldRecenterIdle = true;
			if (LocoComp->OrientationDirection == EOrientationDirection::Forward)
			{
				if (IsDiagonalRight())
					if (LocoComp->bIsCrouched)
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleCrouchRecenterAnims.L_02;
					else
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleRecenterAnims.L_02;
				else
					if (LocoComp->bIsCrouched)
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleCrouchRecenterAnims.R_01;
					else
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleRecenterAnims.R_01;
			}
			else
			{
				if (IsDiagonalRight())
					if (LocoComp->bIsCrouched)
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleCrouchRecenterAnims.R_01;
					else
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleRecenterAnims.R_01;
				else
					if (LocoComp->bIsCrouched)
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleCrouchRecenterAnims.L_02;
					else
						LocoComp->FinalIdleRecenterAnim = LocoComp->IdleRecenterAnims.L_02;
			}
		}
		else LocoComp->bShouldRecenterIdle = false;
	}	
	
	// così Movement Start legge valori freschi anche se il C++ è ancora in Idle
	if (!bPrevShouldMove && bShouldMoveNow)
	{
		LocoComp->SmoothedDir = GetIntendedDir();
		PushOrientationDirection(LocoComp->SmoothedDir);
	}
#pragma endregion
	
	LocoComp->PlayRate = FMath::FInterpTo(LocoComp->PlayRate, LocoComp->TargetPlayRate, DeltaTime, StateData->PlayRateInterpSpeed);

	
#pragma region FALLBACK//Fallback for Jog->Walk (bug - resolved with this) -> might cause bugs in idle
	if (LocoComp->bIsInWalkJogStanceTransition)
	{
		if (!bShouldMoveNow)	// se l'input finisce, l'ABP esce dal MovChangeGait
		{
			LocoComp->bIsInWalkJogStanceTransition = false;
			LocoComp->bShouldWalkJogStanceTransition = false;
		}
		else
		{
			const float Elapsed = PlayerRef->GetWorld()->GetTimeSeconds() - LocoComp->WalkJogTransitionStartTime;
			if (Elapsed > 3.f) LocoComp->bIsInWalkJogStanceTransition = false;
		}
	}
	if (LocoComp->bIsInStanceTransition)
	{
		const float Elapsed = PlayerRef->GetWorld()->GetTimeSeconds() - LocoComp->StanceTransitionStartTime;
		if (Elapsed > 3.f) LocoComp->bIsInStanceTransition = false; 
	}
	SetBrakingForStanceTransition();
#pragma endregion 
	
	UpdateShoulderTest(); // da modificare in futuro, per ora cambia ogni 10 secondi la spalla di Locomotion
	
	CheckPivot();
	
#pragma region DEBUG
	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Magenta,
	FString::Printf(TEXT("Stance: %s"), *UEnum::GetValueAsString(LocoComp->StanceMode)));
	
	GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::Orange,
	FString::Printf(TEXT("Lean  idx: %d  angle: %.1f  MM: %.1f"), LocoComp->LeanStateIndex, LocoComp->LeanAngle, LocoComp->LeanAngleMM));
#pragma endregion DEBUG
}
