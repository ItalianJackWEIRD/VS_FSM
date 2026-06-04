// Fill out your copyright notice in the Description page of Project Settings.


#include "States/RunState.h"

void URunState::OnEnterState(AActor* StateOwner)
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
	
	// Camera
	if (CameraRef) CameraRef->SetCameraMode(RunCameraData);
	
}

void URunState::OnExitState()
{
	Super::OnExitState();
}

void URunState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Idle");
		return;
	}
	if (!AnimInstance->bIsRunning)
	{
		if (PlayerRef->GetVelocity().Size2D() > AnimInstance->MovStopJogSpeedThreshold)
		{
			PlayerRef->StateManager->SwitchStateByKey("Jog");
			return;
		}
		PlayerRef->StateManager->SwitchStateByKey("Walk");
		return;
	}
	if (AnimInstance->OrientationDirection != EOrientationDirection::Forward)
	{
		// Sprint lo puoi fare solo nel cono del fwd -> capita solo se RB ancora premuto quindi passi direttamente a Jog
		PlayerRef->StateManager->SwitchStateByKey("Jog");
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
}

void URunState::UpdateAnimationParameters(float DeltaTime)
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
}
