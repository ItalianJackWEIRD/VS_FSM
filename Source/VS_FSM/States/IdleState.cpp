// Fill out your copyright notice in the Description page of Project Settings.


#include "States/IdleState.h"

void UIdleState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UIdleState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	if (PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
}
