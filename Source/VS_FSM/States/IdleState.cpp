// Fill out your copyright notice in the Description page of Project Settings.


#include "States/IdleState.h"

void UIdleState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UIdleState::ProcessTurnYawCurve()
{
	LastTurnYawCurveValue = TurnYawCurveValue;
	
	const float CurveValue = AnimInstance->GetCurveValue(FName(AnimInstance->TurnYawCurveName));
	
	if (CurveValue < 1.f)
	{
		// Reset Animazione
		TurnYawCurveValue = 0;
		LastTurnYawCurveValue = 0;
	}
	else
	{
		// read remaining °
		const float RemainingTurnYaw = AnimInstance->GetCurveValue(FName(AnimInstance->RemainingTurnYawCurveName));
		
		// Safe divide
		TurnYawCurveValue = (LastTurnYawCurveValue != 0.f) ? RemainingTurnYaw/LastTurnYawCurveValue : 0.f;
		
		// Apply Delta
		if (LastTurnYawCurveValue != 0.f)
		{
			const float Delta = TurnYawCurveValue - LastTurnYawCurveValue;
			AnimInstance->RootYawOffset -= Delta;
		}
	}
	
}

void UIdleState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
}

void UIdleState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);

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
	
	ProcessTurnYawCurve();
	
	// DEBUG TEMPORANEO
	GEngine->AddOnScreenDebugMessage(
		-1,
		0.f,
		FColor::Green,
		FString::Printf(TEXT("RootYawOffset: %.1f | TurnRight = %d | TurnLeft = %d"), AnimInstance->RootYawOffset, AnimInstance->bShouldTurnRight, AnimInstance->bShouldTurnLeft),
		true);
	
	// SWITCHES
	if (PlayerRef->IsMoving())
	{
		PlayerRef->StateManager->SwitchStateByKey("Walk");
	}
}


