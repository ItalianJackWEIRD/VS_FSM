// Fill out your copyright notice in the Description page of Project Settings.

#include "States/TraversalState.h"
#include "Components/TraversalComponent.h"
#include "Data/TraversalDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ULS.h"

void UTraversalState::OnEnterState(AActor* OwnerRef)
{
	// Il base rimette la capsula in piedi (WantsCrouchedCapsule è false): da accucciati ci si alza.
	Super::OnEnterState(OwnerRef);

	LocoComp->LocomotionMode = ELocomotionMode::Traversal;
	LocoComp->bIsCrouched = false;
	LocoComp->ResetFSMTransientState();

	Elapsed = 0.f;
	bExitRequested = false;
	const UTraversalDataAsset* TraversalData = Cast<UTraversalDataAsset>(StateData);
	MaxDuration = TraversalData ? TraversalData->MaxDuration : 4.f;

	if (!Traversal) Traversal = PlayerRef->FindComponentByClass<UTraversalComponent>();

	// Niente componente, o montaggio non partito: esce al primo tick invece di bloccarsi.
	// Mai SwitchStateByKey dentro OnEnterState: il manager annuncerebbe lo stato sbagliato.
	if (!Traversal)
	{
		bExitRequested = true;
		return;
	}
	Traversal->OnTraversalFinished.AddUObject(this, &UTraversalState::HandleTraversalFinished);
	if (!Traversal->PerformTraversal(Traversal->GetLastCheckResult()))
		bExitRequested = true;
}

void UTraversalState::OnExitState()
{
	// Prima i delegate: un Abort a metà vault mette il CMC in Falling, e non deve richiamare questo stato.
	Super::OnExitState();

	if (Traversal)
	{
		Traversal->OnTraversalFinished.RemoveAll(this);
		// Uscita decisa da fuori, a montaggio ancora in corso: lo ferma e ripristina collisioni e movement mode.
		if (Traversal->IsDoingTraversal()) Traversal->AbortTraversal();
	}
}

void UTraversalState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);              // bShouldMove

	if (StateData) UpdateOrientationDirection(DeltaTime);
	UpdateAnimationParameters(DeltaTime);     // velocity nel contratto; il lean va a zero da solo

	Elapsed += DeltaTime;
	if (!bExitRequested && Elapsed >= MaxDuration)
	{
		UE_LOG(LogULS, Warning, TEXT("UTraversalState: watchdog dopo %.1f s, montaggio fermato."), Elapsed);
		bExitRequested = true;
	}

	if (bExitRequested) ExitTraversal();
}

void UTraversalState::HandleTraversalFinished(bool bInterrupted)
{
	// Si esce al prossimo tick, fuori dalla callback del montaggio.
	bExitRequested = true;
}

void UTraversalState::ExitTraversal()
{
	// Watchdog: se il montaggio suona ancora lo si ferma qui, così il movement mode è già quello finale.
	if (Traversal && Traversal->IsDoingTraversal()) Traversal->AbortTraversal();

	// Dopo un vault si cade. Negli altri casi si torna a terra, e Idle passa da sé a Walk o Jog se ti muovi.
	const bool bFalling = CharacterMovementComponent && CharacterMovementComponent->IsFalling();
	StateManager->SwitchStateByKey(bFalling ? TEXT("InAir") : TEXT("Idle"));
}