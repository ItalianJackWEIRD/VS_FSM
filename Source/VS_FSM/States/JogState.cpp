// Fill out your copyright notice in the Description page of Project Settings.


#include "States/JogState.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void UJogState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UJogState::OnCrouch()
{
	Super::OnCrouch();
}

void UJogState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	AnimInstance->SmoothedDir = FVector::ZeroVector;
	PushOrientationDirection(AnimInstance->SmoothedDir);
}

void UJogState::OnExitState()
{
	Super::OnExitState();
}

void UJogState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Idle");
		return;
	}
	if (AnimInstance->bIsRunning && AnimInstance->OrientationDirection == EOrientationDirection::Forward)
	{
		PlayerRef->StateManager->SwitchStateByKey("Run");
		return;
	}
	if (!AnimInstance->bIsJogging)
	{
		AnimInstance->bShouldWalkJogStanceTransition = true;
		AnimInstance->bIsInWalkJogStanceTransition = true;
		AnimInstance->WalkJogTransitionStartTime = PlayerRef->GetWorld()->GetTimeSeconds();
		PlayerRef->StateManager->SwitchStateByKey("Walk");
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
	
	
	if (AnimInstance->bShouldMove || PlayerRef->GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
		UpdateOrientationDirection(DeltaTime);
	
	UpdateAnimationParameters(DeltaTime);

#pragma region DEBUG
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,  FString::Printf(TEXT("Fwd:   %6.1f"), AnimInstance->Fwd));
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,    FString::Printf(TEXT("Bwd:   %6.1f"), AnimInstance->Bwd));
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Cyan,   FString::Printf(TEXT("Left:  %6.1f"), AnimInstance->Left));
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Yellow, FString::Printf(TEXT("Right: %6.1f"), AnimInstance->Right));
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Blue, FString::Printf(TEXT("SmoothedDir: %s"), *AnimInstance->SmoothedDir.ToString()));
	}
#pragma endregion
}

void UJogState::UpdateAnimationParameters(float DeltaTime)
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
	switch (AnimInstance->OrientationDirection)
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
