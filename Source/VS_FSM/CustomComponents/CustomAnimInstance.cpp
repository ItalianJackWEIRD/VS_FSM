// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomAnimInstance.h"
#include "CustomComponents/LocomotionStateComponent.h"

void UCustomAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerRef = Cast<AVS_FSMCharacter>(GetOwningActor());

	EnsureLocoComp();
}

bool UCustomAnimInstance::EnsureLocoComp()
{
	if (LocoComp) return true;

	if (const AActor* Owner = GetOwningActor())
		LocoComp = Owner->FindComponentByClass<ULocomotionStateComponent>();

	if (!LocoComp)
	{
		// Preview dell'editor o character senza componente: non è un crash, ma in PIE è un bug.
		UE_LOG(LogTemp, Warning, TEXT("UCustomAnimInstance: ULocomotionStateComponent non trovato sull'owner."));
		return false;
	}
	return true;
}

void UCustomAnimInstance::AnimNotify_ResetStanceTransition()
{
	if (EnsureLocoComp()) LocoComp->ResetStanceTransition();
}

EStanceMode UCustomAnimInstance::GetStanceMode() const
{
	// Dal componente: è il motivo per cui StanceMode ci è specchiata.
	// Un backend GASP la legge senza conoscere AVS_FSMCharacter.
	if (LocoComp)
		return LocoComp->StanceMode;

	if (PlayerRef)
		return PlayerRef->GetStanceMode();

	UE_LOG(LogTemp, Warning, TEXT("UCustomAnimInstance::GetStanceMode - né LocoComp né PlayerRef validi!"));
	return EStanceMode::Normal; // fallback
}