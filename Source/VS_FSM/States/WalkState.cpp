// Fill out your copyright notice in the Description page of Project Settings.


#include "States/WalkState.h"

void UWalkState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");	
}

EOrientationDirection UWalkState::GetOrientationDirection()
{
	const FVector Velocity = PlayerRef->GetVelocity();
	
	if (Velocity.Size2D() < KINDA_SMALL_NUMBER) return OrientationDirection;
	
	const FVector Forward = PlayerRef->GetActorForwardVector();
	const FVector velDir = Velocity.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, velDir);
	const float CrossZ = FVector::CrossProduct(Forward, velDir).Z;
	
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	
	if (Angle >= -45.f && Angle <= 45.f)   return EOrientationDirection::Forward;
	if (Angle > 45.f  && Angle < 135.f)    return EOrientationDirection::Right;
	if (Angle > -135.f && Angle < -45.f)   return EOrientationDirection::Left;
	return EOrientationDirection::Backward;  // tutto il resto	
}

void UWalkState::TickState(float DeltaTime)
{
	#pragma region Switches
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Idle");
	}
	#pragma endregion
	
	if (IsValid(AnimInstance))
		AnimInstance->OrientationDirection = GetOrientationDirection();
	
}
