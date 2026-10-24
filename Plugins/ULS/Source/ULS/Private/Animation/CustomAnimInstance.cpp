// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/CustomAnimInstance.h"
#include "Components/LocomotionStateComponent.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "ULS.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDebugHostContract(
		TEXT("uls.debug.HostContract"),
		false,
		TEXT("Mostra a schermo i campi che l'host scrive sul componente: bMontageActive e MovingTraversalCurveValue."));
}

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

void UCustomAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	
	if (EnsureLocoComp()) PushToComponent();
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
	LegIKAlpha = LocoComp->IsFSMBackend() ? 1.f : 0.f;
}

void UCustomAnimInstance::PushToComponent()
{
	LocoComp->bMontageActive = IsSlotActive(MontageSlotName);
	LocoComp->MovingTraversalCurveValue = GetCurveValue(MovingTraversalCurveName);
	
#if !UE_BUILD_SHIPPING
	if (GEngine && CVarDebugHostContract.GetValueOnGameThread())
	{
		GEngine->AddOnScreenDebugMessage(40, 0.f, FColor::Cyan, FString::Printf(
			TEXT("Host -> MontageActive %s   MovingTraversal %.2f"),
			LocoComp->bMontageActive ? TEXT("true") : TEXT("false"),
			LocoComp->MovingTraversalCurveValue));
	}
#endif
}

void UCustomAnimInstance::AnimNotify_ResetStanceTransition()
{
	if (EnsureLocoComp()) LocoComp->ResetStanceTransition();
}
