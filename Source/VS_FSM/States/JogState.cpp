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
	
	PushOrientationDirection(AnimInstance->SmoothedDir);
	
	// Camera
	if (CameraRef) CameraRef->SetCameraMode(JogCameraData);
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
	if (AnimInstance->bIsAiming)
	{
		PlayerRef->StateManager->SwitchStateByKey("Aim");
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
