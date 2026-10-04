// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/CustomAnimInstance.h"
#include "Components/LocomotionStateComponent.h"
#include "ULS.h"

void UCustomAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnsureLocoComp();
}

void UCustomAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnsureLocoComp()) return;

	PullFromComponent();	// game thread: stato del componente -> copie lette dal grafo dell'host
}

bool UCustomAnimInstance::EnsureLocoComp()
{
	if (LocoComp) return true;

	if (const AActor* Owner = GetOwningActor())
		LocoComp = Owner->FindComponentByClass<ULocomotionStateComponent>();

	// Nella preview dell'editor il componente non esiste: normale, niente log.
	// In PIE invece è un bug: lo segnaliamo una volta sola.
	if (!LocoComp && !bWarnedMissingComp && GetWorld() && GetWorld()->IsGameWorld())
	{
		UE_LOG(LogULS, Warning, TEXT("UCustomAnimInstance: ULocomotionStateComponent non trovato sull'owner."));
		bWarnedMissingComp = true;
	}
	return LocoComp != nullptr;
}

void UCustomAnimInstance::PullFromComponent()
{
	Trajectory = LocoComp->Trajectory;
	StanceMode = LocoComp->StanceMode;
}

void UCustomAnimInstance::AnimNotify_ResetStanceTransition()
{
	if (EnsureLocoComp()) LocoComp->ResetStanceTransition();
}