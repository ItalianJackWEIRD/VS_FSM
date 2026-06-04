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
	const FTwo_Anims Set = AnimInstance->TurnAnimsStanding;
	
	if (AnimInstance->bShouldTurnLeft) AnimInstance->FinalTurnAnim = Set.L_02;
	else AnimInstance->FinalTurnAnim = Set.R_01;

}

void UIdleState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	AnimInstance->FinalIdleAnim = AnimInstance->IdleAnims.R_01;	
	AnimInstance->FinalStanceTransitionAnim = AnimInstance->StanceTransitionAnims.R_01;
	
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
	
	if (AnimInstance->RootYawMode == ERootYawMode::Accumulate)
	{
		AnimInstance->RootYawOffset += ActorYawDelta * -1.f;
		
		if (FMath::Abs(AnimInstance->RootYawOffset) > AnimInstance->TurnThreshold 
			&& !AnimInstance->bIsInStanceTransition
			&& AnimInstance->bAnimGraphInIdle)
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
	
	UpdateAnimationParameters();
	
#pragma region Break_Idle_Animation
	if (!AnimInstance->bIsIdleBreak)
	{
		TimerToBreakIdle += DeltaTime;
		if (TimerToBreakIdle > 5.f)
		{
			TimerToBreakIdle = 0.f;
			AnimInstance->bShouldIdleBreak = true;
			AnimInstance->bIsIdleBreak = true;
			int index = FMath::RandRange(1,4);

			switch (index)
			{
			case 1:
				AnimInstance->FinalIdleBreakAnim = AnimInstance->IdleBreakAnims.F_01;
				break;
			case 2:
				AnimInstance->FinalIdleBreakAnim = AnimInstance->IdleBreakAnims.B_02;
				break;
			case 3:
				AnimInstance->FinalIdleBreakAnim = AnimInstance->IdleBreakAnims.L_03;
				break;
			case 4:
				AnimInstance->FinalIdleBreakAnim = AnimInstance->IdleBreakAnims.R_04;
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
            AnimInstance->RootYawOffset,
            (int32)AnimInstance->RootYawMode));
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, 
		FString::Printf(TEXT("FinalTurnAnim: %s"), *AnimInstance->FinalTurnAnim->GetName()));


	#pragma endregion	
	
	#pragma region SWITCHES
	if (PlayerRef->IsMoving())
	{
		if (AnimInstance->bIsJogging)
			PlayerRef->StateManager->SwitchStateByKey("Jog");
		else
			PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
	#pragma endregion
}

void UIdleState::UpdateAnimationParameters()
{
	const FVector V = PlayerRef->GetVelocity();
	AnimInstance->Velocity = V;
	AnimInstance->VelocityXY = FVector(V.X, V.Y, 0.f);
}