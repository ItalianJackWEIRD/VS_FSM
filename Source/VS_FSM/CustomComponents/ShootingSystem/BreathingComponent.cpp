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
		ActiveInterpSpeed = FallSpeed;
		StimulusHoldRemaining -= Dt;
		if (StimulusHoldRemaining <= 0.f) bStimulusActive = false;
		return;
	}

	// PULSE - State Machine
	switch (PulsePhase)
	{
		case EPulsePhase::Rest:
			TargetHeight = 0.f;
			ActiveInterpSpeed = FallSpeed;
			NextPulseTimer -= Dt;
			if (NextPulseTimer <= 0.f && PulsePoseHeights.Num() > 0)
			{
				PulseTarget = PulsePoseHeights[FMath::RandRange(0, PulsePoseHeights.Num() - 1)];
				NextPulseTimer = FMath::RandRange(IntervalRange.X, IntervalRange.Y);
	
				DriftRemaining = FMath::RandRange(HoldRange.X, HoldRange.Y);
				// pose 0, the one where startPose (the start point) needs to be 0 and skip rising.
				if (PulseTarget <= KINDA_SMALL_NUMBER)
				{
					PulseStartPose = 0.f;
					PulsePhase = EPulsePhase::Drifting;
				}
				else
				{
					PulseStartPose = PulseTarget * FMath::RandRange(StartFractionRange.X, StartFractionRange.Y);
					PulsePhase = EPulsePhase::Rising;
				}
			}
			break;
		
		case EPulsePhase::Rising:
			TargetHeight = PulseStartPose;
			ActiveInterpSpeed = RiseSpeed;
			if (FMath::IsNearlyEqual(CurrentHeight, PulseStartPose, 0.02f))
				PulsePhase = EPulsePhase::Drifting;
			break;
		
		case EPulsePhase::Drifting:
			TargetHeight = PulseTarget;
			ActiveInterpSpeed = DriftSpeed;
			DriftRemaining -= Dt;
			if (DriftRemaining <= 0.f)
				PulsePhase = EPulsePhase::Falling;
			break;
		
		case EPulsePhase::Falling:
			TargetHeight = 0.f;
			ActiveInterpSpeed = FallSpeed;
			if (CurrentHeight <= 0.05f)
			{
				CurrentHeight = 0.f;
				PulsePhase = EPulsePhase::Rest;
			}
			break;
	}
}

void UBreathingComponent::UpdateAlert(float Dt)
{
	TargetHeight = AlertHeight;   // domani: constraint vari
	ActiveInterpSpeed = FallSpeed;
}

void UBreathingComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(Dt, TickType, ThisTickFunction);
	if (!AnimInstance || !Character) return;

	const bool bAlert = Character->GetStanceMode() == EStanceMode::Alert;
	bAlert ? UpdateAlert(Dt) : UpdateNormal(Dt);

	CurrentHeight = FMath::FInterpTo(CurrentHeight, TargetHeight, Dt, ActiveInterpSpeed);

	AnimInstance->OverlayHeight = CurrentHeight;

	// In Normal il breathing fa anche da visibilità in base al PulsePhase o Stimolo (alpha). In Alert l'alpha lo fa la stance nello ShootingSystem.
	if (Shooting)
	{
		const float Alpha = (PulsePhase != EPulsePhase::Rest || bStimulusActive) ? 1 : 0;
		Shooting->SetBreathingAlpha(bAlert ? 0.f : Alpha);
	}
	
	GEngine->AddOnScreenDebugMessage(99, 0.f, FColor::Cyan, FString::Printf(TEXT("Pulse: %s"), PulsePhase == EPulsePhase::Rest ? TEXT("REST") : PulsePhase == EPulsePhase::Rising ? TEXT("RISING") : PulsePhase == EPulsePhase::Drifting ? TEXT("DRIFTING") : TEXT("FALLING")));
}
