// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/LocoFSMAnimInstance.h"
#include "CustomComponents/LocomotionStateComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/**
	 * Consumo lato grafo. Tocca solo membri del layer, quindi è thread-safe davvero,
	 * non solo per etichetta.
	 *
	 * Se il flag è già false non marca niente: un consumo "a vuoto" arriverebbe al
	 * componente e potrebbe cancellare un riarmo fresco del C++.
	 */
	bool ConsumeLocal(bool& bValue, bool& bPending)
	{
		if (!bValue) return false;
		bValue   = false;
		bPending = true;
		return true;
	}

	/** Game thread: porta sul componente un consumo fatto dal grafo al frame precedente. */
	void FlushConsume(bool& bPending, bool& Source)
	{
		if (!bPending) return;
		Source   = false;
		bPending = false;
	}
}

void ULocoFSMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnsureLocoComp();
}

void ULocoFSMAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnsureLocoComp()) return;

	SyncFeedback();			// 1. scritture e consumi del grafo -> componente
	PullFromComponent();	// 2. stato aggiornato -> membri letti dal grafo
}

void ULocoFSMAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	
	// Game thread, a valutazione finita: ciò che il grafo ha appena scritto arriva
	// subito al componente, e la FSM lo vede al prossimo tick in qualunque ordine.
	if (EnsureLocoComp()) SyncFeedback();
}

bool ULocoFSMAnimInstance::EnsureLocoComp()
{
	if (LocoComp) return true;

	if (const AActor* Owner = GetOwningActor())
		LocoComp = Owner->FindComponentByClass<ULocomotionStateComponent>();

	// Nella preview dell'editor il componente non esiste: normale, niente log.
	if (!LocoComp && !bWarnedMissingComp && GetWorld() && GetWorld()->IsGameWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("ULocoFSMAnimInstance: LocomotionStateComponent non trovato sull'owner."));
		bWarnedMissingComp = true;
	}
	return LocoComp != nullptr;
}

void ULocoFSMAnimInstance::SyncFeedback()
{
	// Del grafo: una direzione sola, grafo -> componente.
	LocoComp->bAnimGraphInIdle    = bAnimGraphInIdle;
	LocoComp->bAnimGraphInMovStop = bAnimGraphInMovStop;
	LocoComp->bAnimGraphInRunStop = bAnimGraphInRunStop;

	// Del C++: il grafo può solo averle consumate.
	// Regge perché la FSM non riarma mai un flag nello stesso frame in cui il grafo
	// lo consuma: guardie e debounce del controller lo impediscono.
	FlushConsume(bPendingClearShouldPivot,         LocoComp->bShouldPivot);
	FlushConsume(bPendingClearShouldRecenterIdle,  LocoComp->bShouldRecenterIdle);
	FlushConsume(bPendingClearIsIdleBreak,         LocoComp->bIsIdleBreak);
	FlushConsume(bPendingConsumeIdleBreak,         LocoComp->bShouldIdleBreak);
	FlushConsume(bPendingConsumeStanceTransition,  LocoComp->bShouldStanceTransition);
	FlushConsume(bPendingConsumeWalkJogTransition, LocoComp->bShouldWalkJogStanceTransition);
}

void ULocoFSMAnimInstance::PullFromComponent()
{
	CharacterMovement           = LocoComp->CharacterMovement;
	StanceMode                  = LocoComp->StanceMode;

	Velocity                    = LocoComp->Velocity;
	VelocityXY                  = LocoComp->VelocityXY;
	bShouldMove                 = LocoComp->bShouldMove;
	bIsCrouched                 = LocoComp->bIsCrouched;
	MovementGait                = LocoComp->MovementGait;
	bIsAiming                   = LocoComp->bIsAiming;

	OrientationDirection        = LocoComp->OrientationDirection;
	OrientationAngle            = LocoComp->OrientationAngle;
	SmoothedDir                 = LocoComp->SmoothedDir;
	Fwd                         = LocoComp->Fwd;
	Bwd                         = LocoComp->Bwd;
	Left                        = LocoComp->Left;
	Right                       = LocoComp->Right;

	LeanAngle                   = LocoComp->LeanAngle;
	LeanStateIndex              = LocoComp->LeanStateIndex;
	bLeftShoulderLocomotion     = LocoComp->bLeftShoulderLocomotion;

	PivotAnim                   = LocoComp->PivotAnim;
	PivotStartTime              = LocoComp->PivotStartTime;
	PivotEndTimeRemaining       = LocoComp->PivotEndTimeRemaining;

	bMovStopCrouched            = LocoComp->bMovStopCrouched;
	bMovStopJogging             = LocoComp->bMovStopJogging;
	bTransitionRunInJog         = LocoComp->bTransitionRunInJog;
	PlayRate                    = LocoComp->PlayRate;

	RootYawOffset               = LocoComp->RootYawOffset;
	LastRootYawOffset           = LocoComp->LastRootYawOffset;
	bShouldTurnLeft             = LocoComp->bShouldTurnLeft;
	bShouldTurnRight            = LocoComp->bShouldTurnRight;
	TurnThreshold               = LocoComp->TurnThreshold;
	TurnAnimElapsedTime         = LocoComp->TurnAnimElapsedTime;
	FinalTurnAnim               = LocoComp->FinalTurnAnim;
	TurnYawCurveName            = LocoComp->TurnYawCurveName;
	RemainingTurnYawCurveName   = LocoComp->RemainingTurnYawCurveName;

	FinalIdleAnim               = LocoComp->FinalIdleAnim;
	FinalIdleBreakAnim          = LocoComp->FinalIdleBreakAnim;
	FinalIdleRecenterAnim       = LocoComp->FinalIdleRecenterAnim;
	FinalStanceTransitionAnim   = LocoComp->FinalStanceTransitionAnim;

	bUseSeparateBrakingFriction = LocoComp->bUseSeparateBrakingFriction;
	BrakingFriction             = LocoComp->BrakingFriction;
	GroundFriction              = LocoComp->GroundFriction;
	BrakingFrictionFactor       = LocoComp->BrakingFrictionFactor;
	BrakingDecelerationWalking  = LocoComp->BrakingDecelerationWalking;
	MinDistanceToDistanceMatch  = LocoComp->MinDistanceToDistanceMatch;

	// Flag del C++ che il grafo legge e consuma (dopo il flush, quindi già aggiornati).
	bShouldPivot                = LocoComp->bShouldPivot;
	bShouldRecenterIdle         = LocoComp->bShouldRecenterIdle;
	bIsIdleBreak                = LocoComp->bIsIdleBreak;
	bIdleBreakTrigger           = LocoComp->bShouldIdleBreak;
	bStanceTransitionTrigger    = LocoComp->bShouldStanceTransition;
	bWalkJogTransitionTrigger   = LocoComp->bShouldWalkJogStanceTransition;
}

/* ---> CONSUMI DAL GRAFO
 * Tutti passano da ConsumeLocal: nessuna scrittura sul componente dal worker thread.
**/

void ULocoFSMAnimInstance::ClearShouldPivot()
{
	ConsumeLocal(bShouldPivot, bPendingClearShouldPivot);
}

void ULocoFSMAnimInstance::ClearShouldRecenterIdle()
{
	ConsumeLocal(bShouldRecenterIdle, bPendingClearShouldRecenterIdle);
}

void ULocoFSMAnimInstance::ClearIsIdleBreak()
{
	ConsumeLocal(bIsIdleBreak, bPendingClearIsIdleBreak);
}

bool ULocoFSMAnimInstance::ShouldIdleBreak()
{
	return ConsumeLocal(bIdleBreakTrigger, bPendingConsumeIdleBreak);
}

bool ULocoFSMAnimInstance::ShouldStanceTransition()
{
	return ConsumeLocal(bStanceTransitionTrigger, bPendingConsumeStanceTransition);
}

bool ULocoFSMAnimInstance::ShouldMovWalkJogStanceTransition()
{
	return ConsumeLocal(bWalkJogTransitionTrigger, bPendingConsumeWalkJogTransition);
}

void ULocoFSMAnimInstance::AnimNotify_ResetStanceTransition()
{
	if (EnsureLocoComp()) LocoComp->ResetStanceTransition();
}