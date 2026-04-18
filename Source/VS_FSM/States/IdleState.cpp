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
	PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
}

// Per ora inutile
void UIdleState::ProcessTurnYawCurve()
{
	LastTurnYawCurveValue = TurnYawCurveValue;
	
	const float CurveValue = AnimInstance->GetCurveValue(FName(AnimInstance->TurnYawCurveName));
	
	if (FMath::Abs(CurveValue) < 1.f)
	{
		// Reset Animazione
		TurnYawCurveValue = 0;
		LastTurnYawCurveValue = 0;
	}
	else
	{
		// read remaining °
		const float RemainingTurnYaw = FMath::Abs(AnimInstance->GetCurveValue(FName(AnimInstance->RemainingTurnYawCurveName)));
		
		// Safe divide !!!
		TurnYawCurveValue = (LastTurnYawCurveValue != 0.f) ? RemainingTurnYaw/LastTurnYawCurveValue : 0.f;
		
		// Apply Delta only during BlendOut
		if (LastTurnYawCurveValue != 0.f && AnimInstance->RootYawMode == ERootYawMode::BlendOut)
		{
			const float DirectionSign = AnimInstance->bShouldTurnLeft ? -1.f : 1.f;
			const float Delta = (TurnYawCurveValue - LastTurnYawCurveValue) * DirectionSign;
			AnimInstance->RootYawOffset -= Delta;
		}
	}
	
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
	
}

void UIdleState::OnExitState()
{
	Super::OnExitState();
}

void UIdleState::TickState(float DeltaTime)
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
		PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
	#pragma endregion
}


