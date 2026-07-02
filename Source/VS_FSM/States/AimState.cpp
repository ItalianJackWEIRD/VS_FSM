// Fill out your copyright notice in the Description page of Project Settings.


#include "States/AimState.h"

void UAimState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UAimState::OnCrouch()
{
	Super::OnCrouch();
	AnimInstance->bIsCrouched = !AnimInstance->bIsCrouched;
	if (CameraRef)
		CameraRef->SetCameraMode(AnimInstance->bIsCrouched ? AimCrouchCameraData : AimCameraData);
}

void UAimState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	// Camera
	if (CameraRef)
		CameraRef->SetCameraMode(AnimInstance->bIsCrouched ? AimCrouchCameraData : AimCameraData);
	
	AnimInstance->bIsJogging = false;
	
}

void UAimState::OnExitState()
{
	Super::OnExitState();

}

void UAimState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!AnimInstance->bIsAiming)
	{
		if (AnimInstance->bIsCrouched)
		{
			if (!PlayerRef->IsMoving()) 
			{
				PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
				return;
			}
			
			PlayerRef->StateManager->SwitchStateByKey("Crouch_Walk");
			return;
		}
		
		if (!PlayerRef->IsMoving())
		{
			PlayerRef->StateManager->SwitchStateByKey("Idle");
			return;
		}
		
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

#pragma endregion
}

/* 
 *  Ricorda nei enter condition di mettere eventualmente i check se sta nei vari stop state in graph */