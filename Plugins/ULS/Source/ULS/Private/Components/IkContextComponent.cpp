// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/IkContextComponent.h"

#include "ULS.h"
#include "Components/LocomotionStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
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
	if (!OwnerCharacter || !LocoComp || !Capsule)
	{
		UE_LOG(LogULS, Warning, TEXT("UIkContextComponent: character o LocomotionStateComponent o capsule mancanti, tick spento."));
		SetComponentTickEnabled(false);
		return;
	}

	// Ordine del tick: CMC -> questo componente -> mesh, come il LocoComp.
	if (CharacterMovement) AddTickPrerequisiteComponent(CharacterMovement);
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		Mesh->AddTickPrerequisiteComponent(this);
	
	// We save the initial radius
	if (const ACharacter* CDO = OwnerCharacter->GetClass()->GetDefaultObject<ACharacter>())
		DefaultCapsuleRadius = CDO->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	
	// We save ref fo the bone from which we start the sweep.
	if (const USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		ShoulderBoneIndex = Mesh->GetBoneIndex(ShoulderBoneHeight);
	if (ShoulderBoneIndex == INDEX_NONE)
		UE_LOG(LogULS, Warning, TEXT("UIkContextComponent: osso %s non trovato, altezza delle spalle dal centro della capsula."), *ShoulderBoneHeight.ToString());
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
	const bool bEnabled = LocoComp->bEnableShoulderAvoidance && LocoComp->bShoulderAvoidanceOnState && !LocoComp->bMontageActive;
	const bool bCapsuleAtRest = Capsule->GetUnscaledCapsuleRadius() >= DefaultCapsuleRadius;
	
	if (!bEnabled && LocoComp->ShoulderAvoidanceWeight == 0.f && bCapsuleAtRest) return;

	if (!bEnabled )
	{
		TargetShoulderWeight = 0.f;
		bShoulderHitLeftPrev = bShoulderHitRightPrev = false;
		LastShoulderDistance = -1.f;
	}
	else
	{
		TargetShoulderWeight = ProbeShoulderTarget();
	}
	
	const float InterpSpeed = !bEnabled ? ShoulderExitSpeed : FMath::Abs(TargetShoulderWeight) > FMath::Abs(LocoComp->ShoulderAvoidanceWeight) ? 
	ShoulderRiseSpeed : ShoulderFallSpeed;
	
	// serve perche altrimeti la weight non snapperebbe a 0.
	float NewWeight = FMath::FInterpTo(LocoComp->ShoulderAvoidanceWeight, TargetShoulderWeight, DeltaTime,	InterpSpeed);
	if (TargetShoulderWeight == 0.f && FMath::Abs(NewWeight) < ShoulderSnapThreshold) NewWeight = 0;
	
	LocoComp->ShoulderAvoidanceWeight = NewWeight;
	LocoComp->ShoulderAvoidanceAlpha = NewWeight != 0.f ? 1.f : 0.f;
	
	UpdateShoulderCapsule(TargetShoulderWeight != 0.f);
	DebugShoulderAvoidance();
}

/** 
 * Check with a Sweep if Something Collides and then return the Weight of the Additive.
 * Checks if there's a wall in front of us and stops the additive in this case.
 */
float UIkContextComponent::ProbeShoulderTarget()
{
	const FVector Vel2D = OwnerCharacter->GetVelocity() * FVector(1.f, 1.f, 0.f);
	const bool bStationary = Vel2D.SizeSquared() < FMath::Square(ShoulderStationarySpeed);
	bLastShoulderStationary = bStationary;
	LastShoulderDistance = -1.f;
	
	// In movimento: sfere ai lati, lanciate lungo la velocità (distanza davanti).
	// Da fermo: sfere dal centro verso l'esterno, ai lati dell'attore (distanza laterale).
	const FVector Dir = bStationary ? FVector::ZeroVector : OwnerCharacter->GetVelocity().GetSafeNormal2D();
	const FVector Right = bStationary ? OwnerCharacter->GetActorRightVector() : FVector::CrossProduct(FVector::UpVector, Dir);
	const float LateralOffset = bStationary ? 0.f : ShoulderLateralOffset;
	const FVector LeftDir  = bStationary ? -Right : Dir;
	const FVector RightDir = bStationary ?  Right : Dir;
	
	FVector Center = Capsule->GetComponentLocation(); // change Z to match bone height
	if (ShoulderBoneIndex != INDEX_NONE)
		Center.Z = OwnerCharacter->GetMesh()->GetBoneTransform(ShoulderBoneIndex).GetLocation().Z;
	
	const FVector LeftStart = Center - Right * LateralOffset;
	const FVector RightStart = Center + Right * LateralOffset;
	
	const float BaseLenght  = bStationary ? FMath::Max(ShoulderZeroDistance - ShoulderSweepRadius, 1.f) : ShoulderZeroDistance;
	const float LeftLenght = ShoulderZeroDistance + (bShoulderHitLeftPrev ? ShoulderHysterisis : 0.f);
	const float RightLenght = ShoulderZeroDistance + (bShoulderHitRightPrev ? ShoulderHysterisis : 0.f);
	
	float OutDistL = 0.f, OutDistR = 0.f;
	
	// Exclusive, Right stops checking for left.
	bool bHitL = !bShoulderHitRightPrev && SweepShoulder(LeftStart, LeftStart + LeftDir * LeftLenght, ShoulderSweepRadius, OutDistL);
	bool bHitR = !bShoulderHitLeftPrev && SweepShoulder(RightStart, RightStart + RightDir * RightLenght, ShoulderSweepRadius, OutDistR); 
	
	// Da fermo la distanza utile è quella laterale dal centro: strada fatta dalla sfera + il suo raggio.
	if (bStationary)
	{
		OutDistL += ShoulderSweepRadius;
		OutDistR += ShoulderSweepRadius;
	}
	
	if (bHitL && bHitR)		// both, rare case. 
	{
		if (OutDistL <= OutDistR) bHitR = false;
		else bHitL = false;
	}
	
	if (!bStationary && (bHitL || bHitR))		// we check if we have a wall in front of us. 
	{
		const float SideDist = bHitR ? OutDistR : OutDistL;
		const float CenterLenght = SideDist + ShoulderSweepRadius + ShoulderWallTolerance;
		float OutDistCenter = 0.f;
		if (SweepShoulder(Center, Center + Dir * CenterLenght, 0.f, OutDistCenter)) 
			bHitR = bHitL = false;
	}
	bShoulderHitLeftPrev = bHitL;
	bShoulderHitRightPrev = bHitR;
	
	if (!bHitL && !bHitR) return 0.f;
	
	const float Dist = bHitL ? OutDistL : OutDistR;
	LastShoulderDistance = Dist;
	const float Weight = FMath::GetMappedRangeValueClamped(FVector2f(ShoulderFullDistance, ShoulderZeroDistance), 
		FVector2f(1.f, 0.f), Dist);
	return bHitR ? Weight : -Weight;
	
}

bool UIkContextComponent::SweepShoulder(const FVector& Start, const FVector& End, float Radius,
	float& OutDistance) const
{
	// Same Channel as CanStandUp.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShoulderAvoidance), false, OwnerCharacter);
	FCollisionResponseParams Response;
	Capsule->InitSweepCollisionParams(Params, Response);
	
	
	FHitResult Hit;
	const ECollisionChannel Channel = Capsule->GetCollisionObjectType();
	const bool bHit = Radius > 0.f
		? GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Radius), Params, Response)
		: GetWorld()->LineTraceSingleByChannel(Hit, Start, End, Channel, Params, Response);

	// If the sphere starts inside an object, Distance = 0.
	OutDistance = bHit ? Hit.Distance : (End - Start).Size();

	DrawShoulderSweep(Start, End, Radius, bHit, Hit.ImpactPoint);
	return bHit;
}

void UIkContextComponent::UpdateShoulderCapsule(bool bWantsNarrow)
{
	// We read the radius every frame: l'uncrouch of CMC put i back at default with no notifies.
	const float CurrentRadius = Capsule->GetUnscaledCapsuleRadius();

	if (bWantsNarrow)
	{
		if (CurrentRadius > ShoulderCapsuleRadius)
			Capsule->SetCapsuleRadius(ShoulderCapsuleRadius);
		return;
	}

	if (CurrentRadius >= DefaultCapsuleRadius) return;	// già larga: niente test, niente scrittura

	// Si riallarga solo se col raggio pieno, nella posizione attuale, c'è spazio. Se no riprova al frame dopo.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShoulderCapsule), false, OwnerCharacter);
	FCollisionResponseParams Response;
	Capsule->InitSweepCollisionParams(Params, Response);

	const FCollisionShape FullShape = FCollisionShape::MakeCapsule(
		DefaultCapsuleRadius * Capsule->GetShapeScale(),
		Capsule->GetScaledCapsuleHalfHeight() - 2.f);	// un filo più bassa, per non toccare il pavimento

	const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
		Capsule->GetComponentLocation(), Capsule->GetComponentQuat(),
		Capsule->GetCollisionObjectType(), FullShape, Params, Response);

	if (!bBlocked)
		Capsule->SetCapsuleRadius(DefaultCapsuleRadius);
}


void UIkContextComponent::DrawShoulderSweep(const FVector& Start, const FVector& End, float Radius, bool bHit, const FVector& HitPoint) const
{
#if ENABLE_DRAW_DEBUG
	if (!CVarDebugShoulderAvoidance.GetValueOnGameThread()) return;

	const FColor Color = bHit ? FColor::Red : FColor::Green;
	DrawDebugLine(GetWorld(), Start, End, Color, false, 0.f, 0, 1.f);
	if (Radius > 0.f)
		DrawDebugSphere(GetWorld(), Start, Radius, 12, Color, false, 0.f, 0, 0.5f);
	if (bHit)
		DrawDebugPoint(GetWorld(), HitPoint, 12.f, FColor::Yellow, false, 0.f);
#endif
}

void UIkContextComponent::DebugShoulderAvoidance() const
{
#if ENABLE_DRAW_DEBUG
	if (!CVarDebugShoulderAvoidance.GetValueOnGameThread()) return;

	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(41, 0.f, FColor::Orange, FString::Printf(
			TEXT("Shoulder  Target %.2f   Weight %.2f   Alpha %.0f   Capsula %.0f / %.0f"),
			TargetShoulderWeight, LocoComp->ShoulderAvoidanceWeight, LocoComp->ShoulderAvoidanceAlpha,
			Radius, DefaultCapsuleRadius));

	// Capsula disegnata solo quando è stretta.
	if (Radius < DefaultCapsuleRadius)
		DrawDebugCapsule(GetWorld(), Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleHalfHeight(),
			Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Orange);
	
	if (GEngine)
	{
		const FString DistText = LastShoulderDistance >= 0.f ? FString::Printf(TEXT("%.0f"), LastShoulderDistance) : TEXT("-");
		GEngine->AddOnScreenDebugMessage(41, 0.f, FColor::Orange, FString::Printf(
			TEXT("Shoulder [%s]  Dist %s   Target %.2f   Weight %.2f   Alpha %.0f   Capsula %.0f / %.0f"),
			bLastShoulderStationary ? TEXT("fermo") : TEXT("moto"), *DistText,
			TargetShoulderWeight, LocoComp->ShoulderAvoidanceWeight, LocoComp->ShoulderAvoidanceAlpha,
			Radius, DefaultCapsuleRadius));
	}
#endif
}
