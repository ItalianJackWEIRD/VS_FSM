// Fill out your copyright notice in the Description page of Project Settings.


#include "States/IdleState.h"


void UIdleState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UIdleState::OnCrouch()
{
	Super::OnCrouch();
	
	RequestStanceTransition("Crouch_Idle");
}

void UIdleState::SelectTurnAnim()
{	
	const FTwo_Anims Set = LocoComp->TurnAnimsStanding;
	
	if (LocoComp->bShouldTurnLeft) LocoComp->FinalTurnAnim = Set.L_02;
	else LocoComp->FinalTurnAnim = Set.R_01;

}

void UIdleState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	LocoComp->FinalIdleAnim = LocoComp->IdleAnims.R_01;	
	LocoComp->FinalStanceTransitionAnim = LocoComp->StanceTransitionAnims.R_01;
	
	// Camera
	if (CameraRef) CameraRef->SetCameraMode(IdleCameraData);
}

void UIdleState::OnExitState()
{
	Super::OnExitState();
}

void UIdleState::TickState(float DeltaTime)
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
	
#pragma region Break_Idle_Animation
	if (!LocoComp->bIsIdleBreak)
	{
		TimerToBreakIdle += DeltaTime;
		if (TimerToBreakIdle > 5.f)
		{
			TimerToBreakIdle = 0.f;
			LocoComp->bShouldIdleBreak = true;
			LocoComp->bIsIdleBreak = true;
			int index = FMath::RandRange(1,4);

			switch (index)
			{
			case 1:
				LocoComp->FinalIdleBreakAnim = LocoComp->IdleBreakAnims.F_01;
				break;
			case 2:
				LocoComp->FinalIdleBreakAnim = LocoComp->IdleBreakAnims.B_02;
				break;
			case 3:
				LocoComp->FinalIdleBreakAnim = LocoComp->IdleBreakAnims.L_03;
				break;
			case 4:
				LocoComp->FinalIdleBreakAnim = LocoComp->IdleBreakAnims.R_04;
				break;
			default:
				break;
			}
		}
	}
#pragma	endregion 
	
	#pragma region DEBUG	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
        FString::Printf(TEXT("ActorYaw: %.1f | RootYawOffset: %.1f | Mode: %d"),
            PlayerRef->GetActorRotation().Yaw,
            LocoComp->RootYawOffset,
            (int32)LocoComp->RootYawMode));
	
	if (LocoComp->FinalTurnAnim)
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
			FString::Printf(TEXT("FinalTurnAnim: %s"), *LocoComp->FinalTurnAnim->GetName()));


	#pragma endregion	
	
	#pragma region SWITCHES
	if (PlayerRef->IsMoving())
	{
		if (LocoComp->MovementGait == EMovementGait::Jog)
			PlayerRef->StateManager->SwitchStateByKey("Jog");
		else
			PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
	if (LocoComp->bIsAiming)
	{
		PlayerRef->StateManager->SwitchStateByKey("Aim");
	}
	#pragma endregion
}