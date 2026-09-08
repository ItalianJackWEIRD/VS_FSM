// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/VSCharacterMovementComponent.h"

float UVSCharacterMovementComponent::GetMaxSpeed() const
{
	const float Base = Super::GetMaxSpeed();

	if (MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking) return Base;
	if (LateralScale >= 1.f && BackwardScale >= 1.f) return Base;

	const FVector Dir = Velocity.GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return Base;

	const FVector Fwd = GetOwner()->GetActorForwardVector();
	const float Dot = FVector::DotProduct(Fwd, Dir);   // +1 avanti, 0 laterale, -1 indietro

	const float Scale = (Dot >= 0.f)
		? FMath::Lerp(LateralScale, 1.f, Dot)              // laterale → avanti
		: FMath::Lerp(LateralScale, BackwardScale, -Dot);  // laterale → indietro

	return Base * Scale;
}
