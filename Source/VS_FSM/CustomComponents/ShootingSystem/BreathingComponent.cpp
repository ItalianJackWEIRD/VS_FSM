// Fill out your copyright notice in the Description page of Project Settings.
// BreathingComponent.cpp
#include "CustomComponents/ShootingSystem/BreathingComponent.h"
#include "CustomComponents/CustomAnimInstance.h"
#include "CustomComponents/ShootingSystem.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "VS_FSMCharacter.h"

UBreathingComponent::UBreathingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBreathingComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AVS_FSMCharacter>(GetOwner());
	Shooting  = GetOwner()->FindComponentByClass<UShootingSystem>();
	if (Character && Character->GetMesh())
		AnimInstance = Cast<UCustomAnimInstance>(Character->GetMesh()->GetAnimInstance());

	NextPulseTimer = FMath::RandRange(IntervalRange.X, IntervalRange.Y);
}

void UBreathingComponent::RequestRaise(float InHeight, float HoldSeconds)
{
	bStimulusActive       = true;
	StimulusHeight        = FMath::Clamp(InHeight, 0.f, 1.f);
	StimulusHoldRemaining = HoldSeconds;
}

void UBreathingComponent::RequestLower()
{
	bStimulusActive = false;
}

void UBreathingComponent::UpdateNormal(float Dt)
{
	if (bStimulusActive)   // lo stimolo ha priorità sui pulse random
	{
		TargetHeight = StimulusHeight;
		StimulusHoldRemaining -= Dt;
		if (StimulusHoldRemaining <= 0.f) bStimulusActive = false;
		return;
	}

	if (bPulseActive)
	{
		TargetHeight = PulseHeight;
		HoldRemaining -= Dt;
		if (HoldRemaining <= 0.f) bPulseActive = false;
	}
	else
	{
		TargetHeight = 0.f;   // arma giù, locomotion guida (alpha 0)
		NextPulseTimer -= Dt;
		if (NextPulseTimer <= 0.f)
		{
			bPulseActive   = true;
			PulseHeight    = FMath::RandRange(PulseHeightRange.X, PulseHeightRange.Y);
			HoldRemaining  = FMath::RandRange(HoldRange.X, HoldRange.Y);
			NextPulseTimer = FMath::RandRange(IntervalRange.X, IntervalRange.Y);  // re-random
		}
	}
}

void UBreathingComponent::UpdateAlert(float Dt)
{
	TargetHeight = AlertHeight;   // domani: constraint vari
}

void UBreathingComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(Dt, TickType, ThisTickFunction);
	if (!AnimInstance || !Character) return;

	const bool bAlert = Character->GetStanceMode() == EStanceMode::Alert;
	bAlert ? UpdateAlert(Dt) : UpdateNormal(Dt);

	CurrentHeight = FMath::FInterpTo(CurrentHeight, TargetHeight, Dt, HeightInterpSpeed);

	AnimInstance->OverlayHeight = CurrentHeight;

	// In Normal il breathing fa anche da visibilità (alpha). In Alert l'alpha lo fa la stance nello ShootingSystem.
	if (Shooting)
		Shooting->SetBreathingAlpha(bAlert ? 0.f : CurrentHeight);
}
