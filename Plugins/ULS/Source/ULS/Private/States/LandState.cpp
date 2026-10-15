// Fill out your copyright notice in the Description page of Project Settings.

#include "States/LandState.h"
#include "Data/LandDataAsset.h"

void ULandState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);

	LocoComp->LocomotionMode = ELocomotionMode::Ground;
	Elapsed = 0.f;

	// DA sbagliato: Duration a 0, esce al primo tick invece di bloccarsi.
	const ULandDataAsset* LandData = Cast<ULandDataAsset>(StateData);
	Duration = LandData ? LandData->HeavyDuration : 0.f;
}

void ULandState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);              // bShouldMove

	UpdateOrientationDirection(DeltaTime);
	UpdateAnimationParameters(DeltaTime);     // il lean va a zero da solo: il base restituisce 0

	Elapsed += DeltaTime;
	if (Elapsed >= Duration)
		StateManager->SwitchStateByKey(TEXT("Idle"));
}