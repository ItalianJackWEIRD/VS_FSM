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
	if (InSmoothedDir.IsNearlyZero()) return;
	
	const FVector Forward = PlayerRef->GetActorForwardVector();
	const FVector Velocity = PlayerRef->GetVelocity();
	
	// --- Segnale FILTRATO -> Decisioni
	const float DotDecision = FVector::DotProduct(Forward, InSmoothedDir);
	const float CrossZDecision = FVector::CrossProduct(Forward, InSmoothedDir).Z;
	const float DecisionAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZDecision, DotDecision));
	
	// --- Segnale GREZZO -> Correzzioni
	float WarpAngle = DecisionAngle;
	if (Velocity.Size2D() >= StateData->MinSpeedForOrientation)	// Calcoliamo WarpAngle, scartiamo il fallback.
	{
		const FVector VelDir = Velocity.GetSafeNormal2D();
		const float Dot = FVector::DotProduct(Forward, VelDir);
		const float CrossZ = FVector::CrossProduct(Forward, VelDir).Z;
		WarpAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	}
	
	// Warping
	AnimInstance->Fwd   = FMath::UnwindDegrees(WarpAngle);
	AnimInstance->Bwd   = FMath::UnwindDegrees(WarpAngle - 180.f);
	AnimInstance->Left  = FMath::UnwindDegrees(WarpAngle + 90.f);
	AnimInstance->Right = FMath::UnwindDegrees(WarpAngle - 90.f);
	
	// Bucketing
	AnimInstance->OrientationAngle = DecisionAngle;
	
	const float AbsAngle = FMath::Abs(DecisionAngle);
	if (AbsAngle <= StateData->ForwardHalfAngle)   AnimInstance->OrientationDirection = EOrientationDirection::Forward;
	else if (AbsAngle >= 180.f - StateData->BackwardHalfAngle)    AnimInstance->OrientationDirection = EOrientationDirection::Backward;
	else if (DecisionAngle >= 0)   AnimInstance->OrientationDirection = EOrientationDirection::Right;
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
		if (Accel.IsNearlyZero()) return; // quasi a 0, scarta tutto
		TargetDir = Accel.GetSafeNormal2D();			
	}
	
	if (AnimInstance->SmoothedDir.IsNearlyZero())
	{
		AnimInstance->SmoothedDir = TargetDir;
		PushOrientationDirection(TargetDir);
		return;
	}
	
	const float CurrentYaw = FMath::RadiansToDegrees(FMath::Atan2(AnimInstance->SmoothedDir.Y, AnimInstance->SmoothedDir.X));
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(TargetDir.Y, TargetDir.X));
	
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	const float Alpha = FMath::Clamp(DeltaTime * StateData->OrientationInterpSpeed, 0.f, 1.f);
	const float NewYaw = FMath::UnwindDegrees(CurrentYaw + DeltaYaw*Alpha);
	
	AnimInstance->SmoothedDir = FRotator(0,NewYaw,0).Vector();
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

void ULocomotionState::UpdateAnimationParameters(float DeltaTime)
{
	// Velocity
	const FVector V = PlayerRef->GetVelocity();
	AnimInstance->Velocity = V;
	AnimInstance->VelocityXY = FVector(V.X, V.Y, 0.f);
	
	// --- LEAN ANGLE SECTION ---
	const float CurrentYaw = PlayerRef->GetActorRotation().Yaw;
	const float ActorYawDelta = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentYaw);
	PreviousActorYaw = CurrentYaw;
	
	// Yaw rate (gradi/secondo)
	const float YawRate = (DeltaTime > KINDA_SMALL_NUMBER) ? ActorYawDelta / DeltaTime : 0.f;
	
	float DirectionSign = 1.f;
	switch (AnimInstance->OrientationDirection)
	{
	case EOrientationDirection::Forward: DirectionSign = 1.f; break;
	case EOrientationDirection::Backward: DirectionSign = -1.f; break;
	case EOrientationDirection::Left: DirectionSign = 1.f; break;
	case EOrientationDirection::Right: DirectionSign = -1.f; break;
	}
	
	const float RawLean = (YawRate / 4.f) * DirectionSign;
	AnimInstance->LeanAngle = FMath::Clamp(RawLean, -45.f, 45.f);
}

void ULocomotionState::UpdateShoulderTest()
{
	if (!AnimInstance || !PlayerRef) return;

	const UWorld* World = PlayerRef->GetWorld();
	if (!World) return;

	constexpr float Interval = 10.f;

	const int32 Slot  = FMath::FloorToInt(World->GetTimeSeconds() / Interval);
	const bool  bLeft = (Slot % 2) == 0;

	if (bLeft == AnimInstance->bLeftShoulderLocomotion) return;   // edge only

	AnimInstance->bLeftShoulderLocomotion = bLeft;

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
	const bool bLeftShoulder = AnimInstance->bLeftShoulderLocomotion;
	
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
	if (!AnimInstance || !PlayerRef || !StateData) return;
	if (AnimInstance->bShouldPivot) return; // aspetto il consumo
	
	const FVector Vel = PlayerRef->GetVelocity();
	if (Vel.Size2D() < StateData->MinSpeedForPivot) return;
	
	const FVector Accel = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
	if (Accel.IsNearlyZero()) return;
	
	if (FVector::DotProduct(Vel.GetSafeNormal2D(), Accel.GetSafeNormal2D()) > StateData->PivotDotThreshold) return;
	
	const FPivotClip* Clip = ResolvePivotClip(AnimInstance->OrientationDirection);
	if (!Clip) return; // Guard, no pivot se non ha anim assgnate.
	
	AnimInstance->PivotAnim = Clip->Anim;
	AnimInstance->PivotStartTime = Clip->StartTime;
	AnimInstance->bShouldPivot = true;
}


void ULocomotionState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	const bool bShouldMoveNow = !PlayerController->IsMovementInputZero();
	
#pragma region MOVSTOP// Edge true→false = we are entering in Mov Stop → freeze gait for Anim Stop -> check if recentering animation is needed
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
#pragma endregion
	
#pragma region FLARE // Sposta in EquipComponent
	if (AnimInstance->bFlare)
		AnimInstance->FlareAlpha = FMath::FInterpTo(AnimInstance->FlareAlpha, 1.f, DeltaTime, AnimInstance->FlareBlendSpeed);
	else
		AnimInstance->FlareAlpha = FMath::FInterpTo(AnimInstance->FlareAlpha, 0.f, DeltaTime, AnimInstance->FlareBlendSpeed);
#pragma endregion 
	
#pragma region LeanAngle
	
	if (CameraRef) CameraRef->SetLeanAngle(AnimInstance->LeanAngle);
	
#pragma endregion
	
#pragma region FALLBACK//Fallback for Jog->Walk (bug - resolved with this) -> might cause bugs in idle
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
#pragma endregion 
	
	UpdateShoulderTest(); // da modificare in futuro, per ora cambia ogni 10 secondi la spalla di Locomotion
	
	CheckPivot();
	
#pragma region DEBUG
	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Magenta,
	FString::Printf(TEXT("Stance: %s"), *UEnum::GetValueAsString(PlayerRef->GetStanceMode())));
	
	const float ActorYaw = PlayerRef->GetActorRotation().Yaw;
	const float VelYaw   = PlayerRef->GetVelocity().Rotation().Yaw;
	const float Gap      = FMath::Abs(FMath::FindDeltaAngleDegrees(ActorYaw, VelYaw));

	GEngine->AddOnScreenDebugMessage(14, 0.f, FColor::Cyan,
		FString::Printf(TEXT("Gap: %.1f   Fwd: %.1f"), Gap, AnimInstance->Fwd));
#pragma endregion DEBUG
}
