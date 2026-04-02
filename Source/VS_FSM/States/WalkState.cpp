// Fill out your copyright notice in the Description page of Project Settings.


#include "States/WalkState.h"

void UWalkState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");	
}

void UWalkState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	if (!PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Idle");
	}
}
