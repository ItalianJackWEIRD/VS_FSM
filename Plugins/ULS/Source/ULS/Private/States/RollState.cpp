// Fill out your copyright notice in the Description page of Project Settings.

#include "States/RollState.h"
#include "Data/RollDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

void URollState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);

	LocoComp->LocomotionMode = ELocomotionMode::Ground;
	Elapsed = 0.f;

	// DA sbagliato: Duration a 0, esce al primo tick invece di bloccarsi.
	const URollDataAsset* RollData = Cast<URollDataAsset>(StateData);
	if (!RollData) { Duration = 0.f; return; }

	Duration = RollData->RollDuration;
	StartRoll(RollData);
}

void URollState::StartRoll(const URollDataAsset* RollData)
{
	if (!RollData->RollMontage) return;

	// Il root motion va verso l'avanti della capsula: girala nella direzione del movimento.
	// Durante il root motion il CMC non la ruota, quindi la capriola resta dritta.
	const FVector Vel2D = CharacterMovementComponent->Velocity.GetSafeNormal2D();
	if (!Vel2D.IsNearlyZero()) PlayerRef->SetActorRotation(Vel2D.Rotation());

	// Parte dal frame di contatto, come la Start Position della FSM.
	if (UAnimInstance* Anim = PlayerRef->GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Play(RollData->RollMontage, 1.f, EMontagePlayReturnType::MontageLength, RollData->RollStartTime);
		bRollPlaying = true;
	}
}

void URollState::OnExitState()
{
	// Il montaggio sfuma: sotto, i backend sono già a terra (Always Update Source Pose sullo Slot).
	if (bRollPlaying)
	{
		const URollDataAsset* RollData = Cast<URollDataAsset>(StateData);
		UAnimInstance* Anim = PlayerRef->GetMesh()->GetAnimInstance();
		if (Anim && RollData) Anim->Montage_Stop(RollData->RollBlendOut, RollData->RollMontage);
		bRollPlaying = false;
	}

	Super::OnExitState();
}

void URollState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);              // bShouldMove

	UpdateOrientationDirection(DeltaTime);
	UpdateAnimationParameters(DeltaTime);     // il lean va a zero da solo: il base restituisce 0

	Elapsed += DeltaTime;
	if (Elapsed >= Duration)
		StateManager->SwitchStateByKey(TEXT("Idle"));
}