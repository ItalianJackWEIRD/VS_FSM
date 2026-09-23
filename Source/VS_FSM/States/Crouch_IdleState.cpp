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
	LocoComp->bIsCrouched = false;
	RequestStanceTransition("Idle");
}

void UCrouch_IdleState::SelectTurnAnim()
{
	const FTwo_Anims Set = LocoComp->TurnAnimsCrouching;
	
	if (LocoComp->bShouldTurnLeft) LocoComp->FinalTurnAnim = Set.L_02;
	else LocoComp->FinalTurnAnim = Set.R_01;
}

void UCrouch_IdleState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	#pragma region YAW_ANIMATION
	const float CurrentYaw = PlayerRef->GetActorRotation().Yaw;
	const float ActorYawDelta = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentYaw);
	PreviousActorYaw = CurrentYaw;
	
	if (LocoComp->RootYawMode == ERootYawMode::Accumulate)
	{
		LocoComp->RootYawOffset += ActorYawDelta * -1.f;
		
		if (FMath::Abs(LocoComp->RootYawOffset) > LocoComp->TurnThreshold 
			&& !LocoComp->bIsInStanceTransition
			&& LocoComp->bAnimGraphInIdle)
		{
			if (LocoComp->RootYawOffset > 0) LocoComp->bShouldTurnLeft = true;
			else LocoComp->bShouldTurnRight = true;
			
			SelectTurnAnim();
			LocoComp->TurnAnimElapsedTime = 0.f;	// Reset Animation
			LocoComp->RootYawMode = ERootYawMode::BlendOut;
		}
	}
	else // BlendOut
	{
		LocoComp->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			LocoComp->RootYawOffset,
			0.f,
			SpringState,
			80.f, // Stiffness
			1.f, // Damping
			DeltaTime
			);
		
		if (FMath::Abs(LocoComp->RootYawOffset) < 0.1f)
		{
			LocoComp->RootYawOffset = 0.f;
			LocoComp->RootYawMode = ERootYawMode::Accumulate;
			LocoComp->bShouldTurnLeft = false;
			LocoComp->bShouldTurnRight = false;
		}
	}
	
	if (LocoComp->FinalTurnAnim != nullptr)
	{
		LocoComp->TurnAnimElapsedTime += DeltaTime;
	}
	#pragma endregion
	
	UpdateAnimationParameters(DeltaTime);
	
	#pragma region DEBUG	

	#pragma endregion	
	
	#pragma region SWITCHES
	if (IsMoving())
	{
		StateManager->SwitchStateByKey("Crouch_Walk");
		return;
	}
	if (LocoComp->bIsAiming)
	{
		StateManager->SwitchStateByKey("Aim");
		return;
	}
	#pragma endregion
}

void UCrouch_IdleState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	LocoComp->bIsCrouched = true;
	// Anims
	LocoComp->FinalIdleAnim = LocoComp->IdleAnims.L_02;
	LocoComp->FinalStanceTransitionAnim = LocoComp->StanceTransitionAnims.L_02;

}

void UCrouch_IdleState::OnExitState()
{
	Super::OnExitState();

}
