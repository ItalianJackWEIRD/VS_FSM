// Fill out your copyright notice in the Description page of Project Settings.


#include "States/Crouch_IdleState.h"

void UCrouch_IdleState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Idle");
}

void UCrouch_IdleState::OnCrouch()
{
	Super::OnCrouch();
	PlayerRef->StateManager->SwitchStateByKey("Idle");
}

void UCrouch_IdleState::SelectTurnAnim()
{
	const FTwo_Anims Set = AnimInstance->TurnAnimsCrouching;
	
	if (AnimInstance->bShouldTurnLeft) AnimInstance->FinalTurnAnim = Set.L_02;
	else AnimInstance->FinalTurnAnim = Set.R_01;
}

void UCrouch_IdleState::TickState(float DeltaTime)
{
	
	#pragma region YAW_ANIMATION
	const float CurrentYaw = PlayerRef->GetActorRotation().Yaw;
	const float ActorYawDelta = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentYaw);
	PreviousActorYaw = CurrentYaw;
	
	if (AnimInstance->RootYawMode == ERootYawMode::Accumulate)
	{
		AnimInstance->RootYawOffset += ActorYawDelta * -1.f;
		
		if (FMath::Abs(AnimInstance->RootYawOffset) > AnimInstance->TurnThreshold)
		{
			if (AnimInstance->RootYawOffset > 0) AnimInstance->bShouldTurnLeft = true;
			else AnimInstance->bShouldTurnRight = true;
			
			SelectTurnAnim();
			AnimInstance->TurnAnimElapsedTime = 0.f;	// Reset Animation
			AnimInstance->RootYawMode = ERootYawMode::BlendOut;
		}
	}
	else // BlendOut
	{
		AnimInstance->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			AnimInstance->RootYawOffset,
			0.f,
			SpringState,
			80.f, // Stiffness
			1.f, // Damping
			DeltaTime
			);
		
		if (FMath::Abs(AnimInstance->RootYawOffset) < 0.1f)
		{
			AnimInstance->RootYawOffset = 0.f;
			AnimInstance->RootYawMode = ERootYawMode::Accumulate;
			AnimInstance->bShouldTurnLeft = false;
			AnimInstance->bShouldTurnRight = false;
		}
	}
	
	if (AnimInstance->FinalTurnAnim != nullptr)
	{
		AnimInstance->TurnAnimElapsedTime += DeltaTime;
	}
	#pragma endregion
	
	#pragma region DEBUG	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
		FString::Printf(TEXT("ActorYaw: %.1f | RootYawOffset: %.1f | Mode: %d"),
			PlayerRef->GetActorRotation().Yaw,
			AnimInstance->RootYawOffset,
			(int32)AnimInstance->RootYawMode));
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, 
		FString::Printf(TEXT("FinalTurnAnim: %s"), *AnimInstance->FinalTurnAnim->GetName()));
	#pragma endregion	
	
	#pragma region SWITCHES
	if (PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Crouch_Walk");
	}
	#pragma endregion
}

void UCrouch_IdleState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	AnimInstance->bIsCrouched = true;
	AnimInstance->FinalIdleAnim = AnimInstance->IdleAnims.L_02;
}

void UCrouch_IdleState::OnExitState()
{
	Super::OnExitState();
	
	AnimInstance->bIsCrouched = false;
}
