// Fill out your copyright notice in the Description page of Project Settings.


#include "States/InAirState.h"
#include "Data/InAirDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void UInAirState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);
	
	LocoComp->LocomotionMode = ELocomotionMode::InAir;
}

void UInAirState::OnExitState()
{
	Super::OnExitState();
}

void UInAirState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
}

void UInAirState::ApplyMovementParameters()
{
	Super::ApplyMovementParameters();
	
	const UInAirDataAsset* AirData = Cast<UInAirDataAsset>(StateData);
	if (!AirData && !CharacterMovementComponent) return;
	
	CharacterMovementComponent->AirControl = AirData->AirControl;
	CharacterMovementComponent->AirControlBoostMultiplier = AirData->AirControlBoostMultiplier;
	CharacterMovementComponent->BrakingDecelerationFalling = AirData->BrakingDecelerationFalling;
	CharacterMovementComponent->FallingLateralFriction = AirData->FallingLateralFriction;	
}

void UInAirState::OnLanded(const FHitResult& Hit)
{
	LocoComp->LandSpeed = FMath::Max(0.f, -CharacterMovementComponent->Velocity.Z);
	
	GEngine->AddOnScreenDebugMessage(22, 3.f, FColor::Cyan,
		FString::Printf(TEXT("Landed: %.0f cm/s"), LocoComp->LandSpeed));
	
	StateManager->SwitchStateByKey("Idle");
}
