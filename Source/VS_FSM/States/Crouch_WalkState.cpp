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
	LocoComp->bIsCrouched = false;
	RequestStanceTransition("Walk");
}

void UCrouch_WalkState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	LocoComp->bIsCrouched = true;
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	LocoComp->LastRootYawOffset = 0.f;
	LocoComp->RootYawMode = ERootYawMode::Accumulate;
	LocoComp->bShouldTurnLeft = false;
	LocoComp->bShouldTurnRight = false;
	
	// Camera
	if (CameraRef && CrouchCameraData) CameraRef->SetCameraMode(CrouchCameraData);
	
}

void UCrouch_WalkState::OnExitState()
{
	Super::OnExitState();
	
}

void UCrouch_WalkState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	#pragma region Switches
	if (!IsMoving())
	{
		StateManager->SwitchStateByKey("Crouch_Idle");
		return;
	}
	if (LocoComp->bIsAiming)
	{
		StateManager->SwitchStateByKey("Aim");
		return;
	}
	if (LocoComp->MovementGait == EMovementGait::Run)
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
}