// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/LocomotionStateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ULocomotionStateComponent::ULocomotionStateComponent()
{
	// Non tickka: è un contenitore. Scrive la FSM, legge l'AnimInstance.
	// Niente tick = nessuna nuova domanda sull'ordine di tick.
	PrimaryComponentTick.bCanEverTick = false;
}

void ULocomotionStateComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshMovementCache();
}

FVector ULocomotionStateComponent::GetAcceleration() const
{
	return CharacterMovement ? CharacterMovement->GetCurrentAcceleration() : FVector::ZeroVector;
}

void ULocomotionStateComponent::RefreshMovementCache()
{
	if (!CharacterMovement)
	{
		if (const AActor* Owner = GetOwner())
			CharacterMovement = Owner->FindComponentByClass<UCharacterMovementComponent>();
	}
	if (!CharacterMovement) return;

	bUseSeparateBrakingFriction = CharacterMovement->bUseSeparateBrakingFriction;
	BrakingFriction             = CharacterMovement->BrakingFriction;
	GroundFriction              = CharacterMovement->GroundFriction;
	BrakingFrictionFactor       = CharacterMovement->BrakingFrictionFactor;
	BrakingDecelerationWalking  = CharacterMovement->BrakingDecelerationWalking;
}

/* ---> TRIGGER ONE-SHOT
 * La FSM arma il flag, il grafo lo consuma leggendolo. Pure per non dover
 * cablare un exec pin in una transition rule.
**/

bool ULocomotionStateComponent::ShouldIdleBreak()
{
	if (bShouldIdleBreak)
	{
		bShouldIdleBreak = false;
		return true;
	}
	return false;
}

bool ULocomotionStateComponent::ShouldStanceTransition()
{
	if (bShouldStanceTransition)
	{
		bShouldStanceTransition = false;
		return true;
	}
	return false;
}

bool ULocomotionStateComponent::ShouldMovWalkJogStanceTransition()
{
	if (bShouldWalkJogStanceTransition)
	{
		bShouldWalkJogStanceTransition = false;
		return true;
	}
	return false;
}

void ULocomotionStateComponent::ResetStanceTransition()
{
	bIsInStanceTransition = false;
}