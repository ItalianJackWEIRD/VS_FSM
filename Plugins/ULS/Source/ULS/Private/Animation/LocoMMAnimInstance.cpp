// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LocoMMAnimInstance.h"
#include "Components/LocomotionStateComponent.h"
#include "ULS.h"

void ULocoMMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnsureLocoComp();
}

void ULocoMMAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnsureLocoComp()) return;

	PullFromComponent();	// stato aggiornato -> membri letti da chooser e nodo MM
}

bool ULocoMMAnimInstance::EnsureLocoComp()
{
	if (LocoComp) return true;

	if (const AActor* Owner = GetOwningActor())
		LocoComp = Owner->FindComponentByClass<ULocomotionStateComponent>();

	// Nella preview dell'editor il componente non esiste: normale, niente log.
	if (!LocoComp && !bWarnedMissingComp && GetWorld() && GetWorld()->IsGameWorld())
	{
		UE_LOG(LogULS, Warning, TEXT("ULocoMMAnimInstance: LocomotionStateComponent non trovato sull'owner."));
		bWarnedMissingComp = true;
	}
	return LocoComp != nullptr;
}

void ULocoMMAnimInstance::PullFromComponent()
{
	Velocity             = LocoComp->Velocity;
	VelocityXY           = LocoComp->VelocityXY;
	Speed2D				 = VelocityXY.Size();
	Acceleration         = LocoComp->GetAcceleration();
	bShouldMove          = LocoComp->bShouldMove;
	bIsCrouched          = LocoComp->bIsCrouched;
	bIsAiming            = LocoComp->bIsAiming;
	MovementGait         = LocoComp->MovementGait;
	StanceMode           = LocoComp->StanceMode;

	OrientationDirection = LocoComp->OrientationDirection;
	LocomotionAngle      = LocoComp->Fwd;

	LeanAngle            = LocoComp->LeanAngle;
	LeanStateIndex       = LocoComp->LeanStateIndex;
	PlayRate             = LocoComp->PlayRate;
	
	bDatabaseCategoryChanged = bShouldMove != bPrevShouldMove || (bShouldMove && MovementGait != PrevGait); // o ti stai fermando o hai cambiato gait in movimento
	bPrevShouldMove = bShouldMove;
	PrevGait = MovementGait;
}