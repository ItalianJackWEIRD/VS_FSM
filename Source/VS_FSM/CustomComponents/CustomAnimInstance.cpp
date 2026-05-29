// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCustomAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	PlayerRef = Cast<AVS_FSMCharacter>(GetOwningActor());
}

bool UCustomAnimInstance::ShouldIdleBreak()
{
	if (bShouldIdleBreak)
	{
		bShouldIdleBreak = false;
		return true;
	}
	return false;
}

bool UCustomAnimInstance::ShouldStanceTransition()
{
	if (bShouldStanceTransition)
	{
		bShouldStanceTransition = false;
		return true;
	}
	return false;
}

void UCustomAnimInstance::AnimNotify_ResetStanceTransition()
{
	bIsInStanceTransition = false;
}

void UCustomAnimInstance::AnimNotify_ResetMovWalkJogChange()
{
	bIsInWalkJogStanceTransition = false;
}

bool UCustomAnimInstance::ShouldMovWalkJogStanceTransition()
{
	if (bShouldWalkJogStanceTransition)
	{
		bShouldWalkJogStanceTransition = false;
		return true;
	}
	return false;
}

void UCustomAnimInstance::RefreshDataAsset()
{
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

EStanceMode UCustomAnimInstance::GetStanceMode() const
{
	if (PlayerRef)
		return PlayerRef->GetStanceMode();
	
	UE_LOG(LogTemp, Warning, TEXT("UCustomAnimInstance::GetStanceMode - PlayerRef is nullptr in ABP!"));
	return EStanceMode::Normal; // fallback
}
