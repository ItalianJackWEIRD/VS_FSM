// Fill out your copyright notice in the Description page of Project Settings.


#include "States/Crouch_WalkState.h"

void UCrouch_WalkState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");	
}

void UCrouch_WalkState::OnCrouch()
{
	Super::OnCrouch();
	
	if (AnimInstance->bIsInStanceTransition) return;
	
	PlayerRef->StateManager->SwitchStateByKey("Walk");
}

void UCrouch_WalkState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	AnimInstance->bIsCrouched = true;
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
}

void UCrouch_WalkState::OnExitState()
{
	Super::OnExitState();
	
	AnimInstance->bIsCrouched = false;
}

void UCrouch_WalkState::TickState(float DeltaTime)
{
	AnimInstance->bShouldMove = !PlayerRef->IsMovementInputZero();
	#pragma region Switches
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
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
	
	UpdateVelocity();
	
	
}

void UCrouch_WalkState::UpdateVelocity()
{
	const FVector V = PlayerRef->GetVelocity();
	AnimInstance->Velocity = V;
	AnimInstance->VelocityXY = FVector(V.X, V.Y, 0.f);
}

void UCrouch_WalkState::UpdateOrientationDirection()		//Also Update values of direction in ABP
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
