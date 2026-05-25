// Fill out your copyright notice in the Description page of Project Settings.

#include "States/WalkState.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void UWalkState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");	
}

void UWalkState::OnCrouch()
{
	Super::OnCrouch();
	
	if (AnimInstance->bIsInStanceTransition) return;
	
	PlayerRef->StateManager->SwitchStateByKey("Crouch_Walk");
}

void UWalkState::OnToggleJog()
{
	Super::OnToggleJog();
	if (AnimInstance->bIsInWalkJogStanceTransition) return;
	
	AnimInstance->bIsJogging = !AnimInstance->bIsJogging;
	
	AnimInstance->bShouldWalkJogStanceTransition = true;
	if (AnimInstance->OrientationDirection == EOrientationDirection::Forward) AnimInstance->bIsInWalkJogStanceTransition = true;
	PlayerRef->StateManager->SwitchStateByKey("Jog");
}

void UWalkState::UpdateOrientationDirection(float DeltaTime)		//Also Update values of direction in ABP -> Now we take accelleration, safer
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
	
	SmoothedDir = FMath::VInterpTo(SmoothedDir, TargetDir, DeltaTime, StateData->OrientationInterpSpeed).GetSafeNormal2D();
	PushOrientationDirection(SmoothedDir);
}

void UWalkState::PushOrientationDirection(FVector InSmoothedDir)
{	
	if (SmoothedDir.IsNearlyZero()) return;
	
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

void UWalkState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	SmoothedDir = FVector::ZeroVector;
	PushOrientationDirection(SmoothedDir);
	
	/*		non funziona benissimo, snappa in tutti tranne forward
	const FVector Pending = PlayerRef->GetPendingMovementInputVector();
	SmoothedDir = Pending.IsNearlyZero()
		? PlayerRef->GetActorForwardVector()
		: Pending.GetSafeNormal2D();
	*/
}

void UWalkState::OnExitState()
{
	Super::OnExitState();
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
		UpdateOrientationDirection(DeltaTime);
	
	UpdateAnimationParameters(DeltaTime);
	
#pragma region DEBUG
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,  FString::Printf(TEXT("Fwd:   %6.1f"), AnimInstance->Fwd));
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,    FString::Printf(TEXT("Bwd:   %6.1f"), AnimInstance->Bwd));
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Cyan,   FString::Printf(TEXT("Left:  %6.1f"), AnimInstance->Left));
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Yellow, FString::Printf(TEXT("Right: %6.1f"), AnimInstance->Right));
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Blue, FString::Printf(TEXT("SmoothedDir: %s"), *SmoothedDir.ToString()));
	}
#pragma endregion
	
	
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
