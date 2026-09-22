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
	
	RequestStanceTransition("Crouch_Walk");
}

void UWalkState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	LocoComp->LastRootYawOffset = 0.f;
	LocoComp->RootYawMode = ERootYawMode::Accumulate;
	LocoComp->bShouldTurnLeft = false;
	LocoComp->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	// Camera
	if (CameraRef) CameraRef->SetCameraMode(WalkCameraData);
	
}

void UWalkState::OnExitState()
{
	Super::OnExitState();
}

void UWalkState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	#pragma region Switches
	if (!IsMoving())
	{
		StateManager->SwitchStateByKey("Idle");
		return;
	}
	if (LocoComp->bIsAiming)
	{
		StateManager->SwitchStateByKey("Aim");
		return;
	}
	if (LocoComp->MovementGait == EMovementGait::Jog)
	{
		LocoComp->bShouldWalkJogStanceTransition = true;
		LocoComp->bIsInWalkJogStanceTransition = true;
		LocoComp->WalkJogTransitionStartTime = PlayerRef->GetWorld()->GetTimeSeconds();
		StateManager->SwitchStateByKey("Jog");
		return;
	}
	if (LocoComp->MovementGait == EMovementGait::Run && LocoComp->OrientationDirection == EOrientationDirection::Forward)
	{
		StateManager->SwitchStateByKey("Run");
		return;
	}
	#pragma endregion
	
	if (FMath::Abs(LocoComp->RootYawOffset) > 0.1f)
	{
		LocoComp->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			LocoComp->RootYawOffset, 0.f, SpringState,
			120.f, 1.f, DeltaTime);
	}
	else
	{
		LocoComp->RootYawOffset = 0.f;
	}
	
	
	if (LocoComp->bShouldMove || PlayerRef->GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
		UpdateOrientationDirection(DeltaTime);
	
	UpdateAnimationParameters(DeltaTime);
	
#pragma region DEBUG
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,  FString::Printf(TEXT("Fwd:   %6.1f"), LocoComp->Fwd));
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,    FString::Printf(TEXT("Bwd:   %6.1f"), LocoComp->Bwd));
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Cyan,   FString::Printf(TEXT("Left:  %6.1f"), LocoComp->Left));
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Yellow, FString::Printf(TEXT("Right: %6.1f"), LocoComp->Right));
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Blue, FString::Printf(TEXT("SmoothedDir: %s"), *LocoComp->SmoothedDir.ToString()));
	}
#pragma endregion
	
	
}
