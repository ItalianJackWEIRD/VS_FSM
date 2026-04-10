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

	const float PlayerYaw = PlayerRef->GetActorRotation().Yaw;
	const float MeshYaw = PlayerRef->GetMesh()->GetComponentRotation().Yaw;
	
	AnimInstance->RootYawOffset = FMath::FindDeltaAngleDegrees(PlayerYaw, MeshYaw);
	
	AnimInstance->bShouldTurnLeft = AnimInstance->RootYawOffset > AnimInstance->TurnThreshold;
	AnimInstance->bShouldTurnRight = AnimInstance->RootYawOffset < -AnimInstance->TurnThreshold;
	
	// DEBUG TEMPORANEO
	GEngine->AddOnScreenDebugMessage(
		-1,
		0.f,
		FColor::Green,
		FString::Printf(TEXT("RootYawOffset: %.1f"), AnimInstance->RootYawOffset),
		true);
	
	// SWITCHES
	if (PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
}
