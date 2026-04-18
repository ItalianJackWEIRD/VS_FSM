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
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Crouching");	
}

void UCrouch_WalkState::TickState(float DeltaTime)
{
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
	}
}
