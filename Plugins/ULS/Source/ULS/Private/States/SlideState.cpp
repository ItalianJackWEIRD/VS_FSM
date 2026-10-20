// Fill out your copyright notice in the Description page of Project Settings.

#include "States/SlideState.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Data/SlideDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void USlideState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);

	LocoComp->LocomotionMode = ELocomotionMode::Ground;
	Elapsed = 0.f;

	// DA sbagliato: Duration a 0, esce al primo tick invece di bloccarsi.
	const USlideDataAsset* SlideData = Cast<USlideDataAsset>(StateData);
	if (!SlideData) { Duration = 0.f; return; }

	Duration = SlideData->RollDuration;
	StartSlide(SlideData);
	PlayerRef->SetAnimRootMotionTranslationScale(SlideData->PushBoostScale);
}

void USlideState::StartSlide(const URollDataAsset* SlideData)
{
	if (!SlideData->RollMontage) return;

	// Il root motion va verso l'avanti della capsula: girala nella direzione del movimento.
	// Durante il root motion il CMC non la ruota, quindi la capriola resta dritta.
	const FVector Vel2D = CharacterMovementComponent->Velocity.GetSafeNormal2D();
	if (!Vel2D.IsNearlyZero()) PlayerRef->SetActorRotation(Vel2D.Rotation());

	// Parte dal frame di contatto, come la Start Position della FSM.
	if (UAnimInstance* Anim = PlayerRef->GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Play(SlideData->RollMontage, 1.f, EMontagePlayReturnType::MontageLength, SlideData->RollStartTime);
		bSlidePlaying = true;
	}
}

void USlideState::OnExitState()
{
	// Il montaggio sfuma: sotto, i backend sono già a terra (Always Update Source Pose sullo Slot).
	if (bSlidePlaying)
	{
		const URollDataAsset* SlideData = Cast<URollDataAsset>(StateData);
		UAnimInstance* Anim = PlayerRef->GetMesh()->GetAnimInstance();
		if (Anim && SlideData) Anim->Montage_Stop(SlideData->RollBlendOut, SlideData->RollMontage);
		bSlidePlaying = false;
	}
	
	PlayerRef->SetAnimRootMotionTranslationScale(1.f);   // obbligatorio: la scala resta sul character e varrebbe anche per la roll
	
	Super::OnExitState();
}

void USlideState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);              // bShouldMove

	UpdateOrientationDirection(DeltaTime);
	UpdateAnimationParameters(DeltaTime);     // il lean va a zero da solo: il base restituisce 0

	// La spinta parte forte e torna normale in PushBoostDuration.
	if (const USlideDataAsset* SlideData = Cast<USlideDataAsset>(StateData))
	{
		const float Alpha = SlideData->PushBoostDuration > 0.f
			? FMath::Clamp(Elapsed / SlideData->PushBoostDuration, 0.f, 1.f) : 1.f;
		PlayerRef->SetAnimRootMotionTranslationScale(FMath::Lerp(SlideData->PushBoostScale, 1.f, Alpha));
	}
	
	Elapsed += DeltaTime;
	if (Elapsed >= Duration)
	{
		if (!LocoComp->bShouldMove)
			StateManager->SwitchStateByKey(TEXT("Crouch_Idle"));   // come la roll, col flag del crouch come l'hai fatto lì
		else
			StateManager->SwitchStateByKey(TEXT("Crouch_Walk"));   // sotto un ostacolo: resti basso
	}
}