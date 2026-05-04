// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UCustomAnimInstance::ShouldStanceTransition()
{
	if (bShouldStanceTransition)
	{
		bShouldStanceTransition = false;
		return true;
	}
	else 
		return false;
}

void UCustomAnimInstance::AnimNotify_ResetStanceTransition()
{
	bIsInStanceTransition = false;
}

void UCustomAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (ACharacter* Char = Cast<ACharacter>(TryGetPawnOwner()))
	{
		CharacterMovement = Char->GetCharacterMovement();
		
		bUseSeparateBrakingFriction = CharacterMovement->bUseSeparateBrakingFriction;
		BrakingFriction             = CharacterMovement->BrakingFriction;
		GroundFriction              = CharacterMovement->GroundFriction;
		BrakingFrictionFactor       = CharacterMovement->BrakingFrictionFactor;
		BrakingDecelerationWalking  = CharacterMovement->BrakingDecelerationWalking;
	}
}
