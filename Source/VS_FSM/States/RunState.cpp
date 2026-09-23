// Fill out your copyright notice in the Description page of Project Settings.


#include "States/RunState.h"


void URunState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	LocoComp->LastRootYawOffset = 0.f;
	LocoComp->RootYawMode = ERootYawMode::Accumulate;
	LocoComp->bShouldTurnLeft = false;
	LocoComp->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	PushOrientationDirection(LocoComp->SmoothedDir);
	
	// Camera
	if (CameraRef) CameraRef->SetCameraMode(RunCameraData);
	
}

void URunState::OnExitState()
{	
	Super::OnExitState();
}

void URunState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!IsMoving())
	{
		StateManager->SwitchStateByKey("Idle");
		return;
	}
	if (LocoComp->MovementGait != EMovementGait::Run)
	{
		if (PlayerRef->GetVelocity().Size2D() > LocoComp->MovStopJogSpeedThreshold)
		{
			StateManager->SwitchStateByKey("Jog");
			return;
		}
		StateManager->SwitchStateByKey("Walk");
		return;
	}
	if (LocoComp->OrientationDirection != EOrientationDirection::Forward)
	{
		// Sprint lo puoi fare solo nel cono del fwd -> capita solo se RB ancora premuto quindi passi direttamente a Jog
		StateManager->SwitchStateByKey("Jog");
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
