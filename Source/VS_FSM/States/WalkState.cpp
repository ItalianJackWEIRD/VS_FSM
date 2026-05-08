// Fill out your copyright notice in the Description page of Project Settings.


#include "States/WalkState.h"

void UWalkState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");	
}

void UWalkState::UpdateOrientationDirection()		//Also Update values of direction in ABP
{
	const FVector Velocity = PlayerRef->GetVelocity();
	
	if (Velocity.Size2D() < KINDA_SMALL_NUMBER)	
	{
		AnimInstance->OrientationAngle = 0.f;
		AnimInstance->OrientationDirection = OrientationDirection; // FALLBACK FORWARD, non viene aggiornato, potrebbe causare bug !
		return;
	} 
	
	const FVector Forward = PlayerRef->GetActorForwardVector();
	const FVector VelDir = Velocity.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, VelDir);
	const float CrossZ = FVector::CrossProduct(Forward, VelDir).Z;
	
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	
	AnimInstance->OrientationAngle = Angle;
	AnimInstance->Fwd   = FMath::UnwindDegrees(Angle);
	AnimInstance->Bwd   = FMath::UnwindDegrees(Angle - 180.f);
	AnimInstance->Left  = FMath::UnwindDegrees(Angle + 90.f);
	AnimInstance->Right = FMath::UnwindDegrees(Angle - 90.f);
	
	if (Angle >= -45.f && Angle <= 45.f)   AnimInstance->OrientationDirection = EOrientationDirection::Forward;
	else if (Angle > 45.f  && Angle < 135.f)    AnimInstance->OrientationDirection = EOrientationDirection::Right;
	else if (Angle > -135.f && Angle < -45.f)   AnimInstance->OrientationDirection = EOrientationDirection::Left;
	else AnimInstance->OrientationDirection = EOrientationDirection::Backward;  // tutto il resto	
}

void UWalkState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	//AnimInstance->bShouldMove = true;
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
}

void UWalkState::OnExitState()
{
	Super::OnExitState();
	//AnimInstance->bShouldMove = false;
}

void UWalkState::TickState(float DeltaTime)
{
	AnimInstance->bShouldMove = !PlayerRef->IsMovementInputZero();
	#pragma region Switches
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Idle");
		return;
	}
	#pragma endregion
	
	if (FMath::Abs(AnimInstance->RootYawOffset) > 0.1f)
	{
		AnimInstance->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			AnimInstance->RootYawOffset, 0.f, SpringState,
			120.f, 1.f, DeltaTime);
	}
	else
	{
		AnimInstance->RootYawOffset = 0.f;
	}
	
	if (IsValid(AnimInstance))
		UpdateOrientationDirection();
	
	UpdateAnimationParameters(DeltaTime);
	
	GEngine->AddOnScreenDebugMessage(-1, 4.0f,FColor::Emerald, FString::Printf(TEXT("V: %.1f | InputZero: %d | bShouldMove: %d"),
			PlayerRef->GetVelocity().Size2D(),
			PlayerRef->IsMovementInputZero() ? 1 : 0,
			AnimInstance->bShouldMove ? 1 : 0)
			);
	
}

void UWalkState::UpdateAnimationParameters(float DeltaTime)
{
	// Velocity
	const FVector V = PlayerRef->GetVelocity();
	AnimInstance->Velocity = V;
	AnimInstance->VelocityXY = FVector(V.X, V.Y, 0.f);
	
	//Lean Angle
	const float CurrentYaw = PlayerRef->GetActorRotation().Yaw;
	const float ActorYawDelta = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentYaw);
	PreviousActorYaw = CurrentYaw;
	
	// Yaw rate (gradi/secondo)
	const float YawRate = (DeltaTime > KINDA_SMALL_NUMBER) ? ActorYawDelta / DeltaTime : 0.f;
	
	float DirectionSign = 1.f;
	switch (OrientationDirection)
	{
		case EOrientationDirection::Forward: DirectionSign = 1.f; break;
		case EOrientationDirection::Backward: DirectionSign = -1.f; break;
		case EOrientationDirection::Left: DirectionSign = 1.f; break;
		case EOrientationDirection::Right: DirectionSign = -1.f; break;
	}
	
	const float RawLean = (YawRate / 4.f) * DirectionSign;
	AnimInstance->LeanAngle = FMath::Clamp(RawLean, -45.f, 45.f);
	
	/* Nel caso volessi interpolare il valore
	AnimInstance->LeanAngle = FMath::FInterpTo(
		AnimInstance->LeanAngle, 
		FMath::Clamp(RawLean, -45.f, 45.f),
		DeltaTime, 
		8.f
		);
	 */ 
}
