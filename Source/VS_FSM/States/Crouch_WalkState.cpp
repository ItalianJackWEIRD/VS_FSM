// Fill out your copyright notice in the Description page of Project Settings.


#include "States/Crouch_WalkState.h"

void UCrouch_WalkState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	if (!PlayerRef->IsMoving())
		PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
}
