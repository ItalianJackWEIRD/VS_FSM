// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/IkContextComponent.h"

#include "ULS.h"
#include "Components/LocomotionStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDebugShoulderAvoidance(
		TEXT("uls.ik.DebugShoulderAvoidance"),
		false,
		TEXT("Disegna le sweep della shoulder avoidance."));
}

UIkContextComponent::UIkContextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIkContextComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		LocoComp          = OwnerCharacter->FindComponentByClass<ULocomotionStateComponent>();
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
		Capsule           = OwnerCharacter->GetCapsuleComponent();
	}

	// Senza character o contratto non c'è niente da leggere né dove scrivere: il tick non serve.
	if (!OwnerCharacter || !LocoComp)
	{
		UE_LOG(LogULS, Warning, TEXT("UIkContextComponent: character o LocomotionStateComponent mancanti, tick spento."));
		SetComponentTickEnabled(false);
		return;
	}

	// Ordine del tick: CMC -> questo componente -> mesh, come il LocoComp.
	if (CharacterMovement) AddTickPrerequisiteComponent(CharacterMovement);
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		Mesh->AddTickPrerequisiteComponent(this);
}

void UIkContextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!LocoComp) return;

	UpdateShoulderAvoidance(DeltaTime);
	// Prossime meccaniche: una chiamata ciascuna, ognuna con il suo early return.
}

void UIkContextComponent::UpdateShoulderAvoidance(float DeltaTime)
{
	// 1. Accesa = interruttore && permesso && !bMontageActive, tutti dal LocoComp.
	// 2. Spenta e a riposo (valore a 0, capsula al raggio normale): return.
	// 3. Target: 0 se spenta, da fermo o senza input, senza sweep.
	//    Altrimenti le due sweep e la distanza -> target fra -1 e 1.
	// 4. Interpolazione, snap a 0, alpha, capsula, scrittura di valore e alpha nel LocoComp.
	
	const bool bEnabled = LocoComp->bEnableShoulderAvoidance && LocoComp->bShoulderAvoidanceOnState && !LocoComp->bMontageActive;
	
	if (!bEnabled && LocoComp->ShoulderAvoidanceWeight == 0.f) return;
	
	if (!bEnabled || !LocoComp->bShouldMove || OwnerCharacter->GetVelocity().Size2D() < ShoulderMinSpeed) TargetShoulderWeight = 0.f;
	else
	{
		// Probe 
		TargetShoulderWeight = 1.f;
	}
	
	const float InterpSpeed = !bEnabled ? ShoulderExitSpeed : FMath::Abs(TargetShoulderWeight) > FMath::Abs(LocoComp->ShoulderAvoidanceWeight) ? 
	ShoulderRiseSpeed : ShoulderFallSpeed;
	
	// serve perche altrimeti la weight non snapperebbe a 0.
	float NewWeight = FMath::FInterpTo(LocoComp->ShoulderAvoidanceWeight, TargetShoulderWeight, DeltaTime,	InterpSpeed);
	if (TargetShoulderWeight == 0.f && FMath::Abs(NewWeight) < ShoulderSnapThreshold) NewWeight = 0;
	
	LocoComp->ShoulderAvoidanceWeight = NewWeight;
	LocoComp->ShoulderAvoidanceAlpha = NewWeight != 0.f ? 1.f : 0.f;
	
	
#if !UE_BUILD_SHIPPING
	// Disegno delle sweep, se CVarDebugShoulderAvoidance è acceso.
#endif
}