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
	
	TAutoConsoleVariable<bool> CVarDebugWallTouch(
		TEXT("uls.ik.DebugWallTouch"),
		false,
		TEXT("Draws the wall touch probes, target and elbow pole, and prints its state."));
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
	
	// Random timers: a fixed seed repeats the same sequence.
	Random.Initialize(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	// Wall touch: upper arm indices and arm lengths. Bone lengths don't change with the pose.
	if (const USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		const FName Upper[2] = { WallTouch.UpperArmL, WallTouch.UpperArmR };
		const FName Lower[2] = { WallTouch.LowerArmL, WallTouch.LowerArmR };
		const FName Hand[2]  = { WallTouch.HandL,     WallTouch.HandR };
		for (int32 i = 0; i < 2; ++i)
		{
			UpperArmIndex[i] = Mesh->GetBoneIndex(Upper[i]);
			if (UpperArmIndex[i] == INDEX_NONE || Mesh->GetBoneIndex(Lower[i]) == INDEX_NONE || Mesh->GetBoneIndex(Hand[i]) == INDEX_NONE)
			{
				UE_LOG(LogULS, Warning, TEXT("UIkContextComponent: arm bones %s / %s / %s not found, wall touch off on that side."),
					*Upper[i].ToString(), *Lower[i].ToString(), *Hand[i].ToString());
				UpperArmIndex[i] = INDEX_NONE;
				continue;
			}
			ArmLength[i] = FVector::Dist(Mesh->GetBoneLocation(Upper[i]), Mesh->GetBoneLocation(Lower[i]))
						 + FVector::Dist(Mesh->GetBoneLocation(Lower[i]), Mesh->GetBoneLocation(Hand[i]));
		}
	}
}

void UIkContextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!LocoComp) return;
	
	UpdateWallTouch(DeltaTime); // it can switch off other mechanics, that's why it's first
	UpdateShoulderAvoidance(DeltaTime);

}

#pragma region AVOIDANCE
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
	// A wall to walk along is not an obstacle: the surface keeps going ahead or behind.
	if (bHitL || bHitR)
	{
		const FVector Forward = bStationary ? OwnerCharacter->GetActorForwardVector() : Dir;
		const FVector SideDir = bHitR ? Right : -Right;
		const float HitForward = bStationary ? 0.f : (bHitR ? OutDistR : OutDistL);
		const float LateralReach = (bStationary ? ShoulderZeroDistance : ShoulderLateralOffset + ShoulderSweepRadius) + 10.f; // small margin
		if (IsAlongWall(Center, Forward, SideDir, HitForward, LateralReach))
			bHitL = bHitR = false;
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
	// We read the radius every frame: l'uncrouch of CMC put it back at default with no notifies.
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

bool UIkContextComponent::IsAlongWall(const FVector& Center, const FVector& Forward, const FVector& SideDir, float HitForward, float LateralReach) const
{
	// A narrow passage is never a wall: if the other side is blocked too, the capsule has to shrink to get through.
	// Checked a sweep radius past the hit, so the trace is inside the passage and not on its edge.
	{
		const FVector Origin = Center + Forward * (HitForward + ShoulderSweepRadius);
		float Dist = 0.f;
		if (SweepShoulder(Origin, Origin - SideDir * ShoulderNarrowPassageHalfWidth, 0.f, Dist))
			return false;
	}

	// A wall keeps going without gaps, ahead or behind: sample it every ShoulderAlongWallStep up to ShoulderAlongWallLength.
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(ShoulderAlongWallLength / ShoulderAlongWallStep));
	for (const float Sign : { 1.f, -1.f })
	{
		bool bContinuous = true;
		for (int32 i = 1; i <= Steps && bContinuous; ++i)
		{
			const FVector Origin = Center + Forward * (HitForward + Sign * ShoulderAlongWallLength * i / Steps);
			float Dist = 0.f;
			bContinuous = SweepShoulder(Origin, Origin + SideDir * LateralReach, 0.f, Dist);
		}
		if (bContinuous) return true;
	}
	return false;
}
#pragma endregion 

#pragma region WALL_TOUCH
void UIkContextComponent::UpdateWallTouch(float DeltaTime)
{
	FWallTouchRuntime& S = WallTouchState;
	const FWallTouchSettings& C = WallTouch;

	const bool bEnabled = LocoComp->bEnableWallTouch && LocoComp->bWallTouchOnState && !LocoComp->bMontageActive;

	const FVector Vel2D = OwnerCharacter->GetVelocity() * FVector(1.f, 1.f, 0.f);
	const float Speed = Vel2D.Size();
	const bool bMoving = Speed >= C.MinSpeed;

	// At rest: inactive and nothing that could start a push. The channels are already zero.
	if (S.State == EWallTouchState::Inactive && (!bEnabled || !bMoving)) return;

	if (bMoving) S.Forward = Vel2D / Speed;
	S.StateTime += DeltaTime;

	switch (S.State)
	{
	case EWallTouchState::Inactive:
	{
		// Probe where the hand will land at the end of the reach.
		const float Predict = Speed * C.ReachDuration;
		FVector TargetL, NormalL, TargetR, NormalR;
		const bool bL = ProbeWallTouch(-1, S.Forward, Predict, TargetL, NormalL);
		const bool bR = ProbeWallTouch(+1, S.Forward, Predict, TargetR, NormalR);
		if (!bL && !bR) break;

		// Walls on both sides (corridor): the closer one wins.
		const bool bRight = bR && (!bL
			|| FVector::DistSquared(TargetR, GetUpperArmLocation(1)) <= FVector::DistSquared(TargetL, GetUpperArmLocation(-1)));
		S.Side       = bRight ? 1 : -1;
		S.Target     = bRight ? TargetR : TargetL;
		S.WallNormal = bRight ? NormalR : NormalL;
		SetWallTouchState(EWallTouchState::Reach);
		break;
	}
	case EWallTouchState::Reach:
	case EWallTouchState::Contact:
	{
		// Committed while the target stays valid. Short grace for door frames and gaps.
		const bool bReach = S.State == EWallTouchState::Reach;
		const float Predict = bReach ? Speed * FMath::Max(C.ReachDuration - S.StateTime, 0.f) : 0.f;
		FVector NewTarget, NewNormal;
		const bool bValid = bEnabled && bMoving && ProbeWallTouch(S.Side, S.Forward, Predict, NewTarget, NewNormal);
		S.LostTime = bValid ? 0.f : S.LostTime + DeltaTime;

		if (!bEnabled || !bMoving || S.LostTime > C.TargetLostGrace)
		{
			SetWallTouchState(EWallTouchState::Release);
			break;
		}
		if (bValid) S.WallNormal = NewNormal;

		if (bReach)
		{
			// One timeline: the pose leads, the IK starts while the pose is still rising.
			if (bValid) S.Target = FMath::VInterpTo(S.Target, NewTarget, DeltaTime, C.ReachTargetSpeed);
			const float T = FMath::Clamp(S.StateTime / C.ReachDuration, 0.f, 1.f);
			S.PoseAlpha = FMath::SmoothStep(0.f, C.PoseLeadEnd, T);
			S.IKAlpha   = FMath::SmoothStep(C.IKStart, 1.f, T);
			if (T >= 1.f) SetWallTouchState(EWallTouchState::Contact);
		}
		else
		{
			// The push: the hand barely follows the wall point (high friction) while the body moves on.
			if (bValid) S.Anchor = FMath::VInterpTo(S.Anchor, NewTarget, DeltaTime, C.ContactSlideSpeed);
			S.Target = S.Anchor;

			const FVector Shoulder = GetUpperArmLocation(S.Side);
			const bool bBehind = FVector::DotProduct(S.Anchor - Shoulder, S.Forward) < -C.BehindLimit;
			const bool bTooFar = FVector::Dist(S.Anchor, Shoulder) > ArmLength[ArmIndex(S.Side)];
			if (S.StateTime >= S.Duration || bBehind || bTooFar)
				SetWallTouchState(EWallTouchState::Release);
		}
		break;
	}
	case EWallTouchState::Release:
	{
		// Mirror of the reach, from the current values: the IK leaves first, then the pose.
		const float T = FMath::Clamp(S.StateTime / C.ReleaseDuration, 0.f, 1.f);
		S.IKAlpha   = S.ReleaseIKFrom   * (1.f - FMath::SmoothStep(0.f, C.PoseLeadEnd, T));
		S.PoseAlpha = S.ReleasePoseFrom * (1.f - FMath::SmoothStep(C.IKStart, 1.f, T));
		if (T >= 1.f) SetWallTouchState(bEnabled ? EWallTouchState::Cooldown : EWallTouchState::Inactive);
		break;
	}
	case EWallTouchState::Cooldown:
		if (!bEnabled || S.StateTime >= S.Duration) SetWallTouchState(EWallTouchState::Inactive);
		break;
	}

	WriteWallTouchChannels();
	DebugWallTouch();
}

void UIkContextComponent::SetWallTouchState(EWallTouchState NewState)
{
	// The only place that changes the wall touch state: the switch decides, this executes.
	FWallTouchRuntime& S = WallTouchState;
	S.State = NewState;
	S.StateTime = 0.f;
	S.LostTime = 0.f;

	switch (NewState)
	{
	case EWallTouchState::Inactive:
		S.Side = 0;
		S.PoseAlpha = S.IKAlpha = 0.f;
		break;
	case EWallTouchState::Contact:
		S.Anchor = S.Target;
		S.Duration = Random.FRandRange(static_cast<float>(WallTouch.ContactDuration.X), static_cast<float>(WallTouch.ContactDuration.Y));
		break;
	case EWallTouchState::Release:
		S.ReleasePoseFrom = S.PoseAlpha;
		S.ReleaseIKFrom = S.IKAlpha;
		break;
	case EWallTouchState::Cooldown:
		S.PoseAlpha = S.IKAlpha = 0.f;
		S.Duration = Random.FRandRange(static_cast<float>(WallTouch.Cooldown.X), static_cast<float>(WallTouch.Cooldown.Y));
		break;
	default:
		break;
	}

#if !UE_BUILD_SHIPPING
	if (CVarDebugWallTouch.GetValueOnGameThread())
		UE_LOG(LogULS, Log, TEXT("WallTouch -> %s  side %d  duration %.2f"), *UEnum::GetValueAsString(NewState), S.Side, S.Duration);
#endif
}

bool UIkContextComponent::ProbeWallTouch(int32 Side, const FVector& Forward, float PredictDistance, FVector& OutTarget, FVector& OutNormal) const
{
	const int32 Arm = ArmIndex(Side);
	if (UpperArmIndex[Arm] == INDEX_NONE) return false;

	const FWallTouchSettings& C = WallTouch;
	const FVector SideDir = FVector::CrossProduct(FVector::UpVector, Forward) * static_cast<float>(Side);

	// Where the shoulder will be when the hand lands, and the hand height on the wall.
	const FVector Shoulder = GetUpperArmLocation(Side) + Forward * PredictDistance;
	const FVector Start = Shoulder + Forward * C.LeadDistance - FVector::UpVector * C.HandHeightBelowShoulder;
	const FVector End   = Start + SideDir * ArmLength[Arm];

	// Same channel and responses as the capsule, the character ignored.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WallTouch), false, OwnerCharacter);
	FCollisionResponseParams Response;
	Capsule->InitSweepCollisionParams(Params, Response);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
		Capsule->GetCollisionObjectType(), FCollisionShape::MakeSphere(C.ProbeRadius), Params, Response);

	// A wall: almost vertical, facing us, not already inside the probe.
	bool bValid = bHit && !Hit.bStartPenetrating
		&& FMath::Abs(Hit.ImpactNormal.Z) <= C.MaxNormalZ
		&& FVector::DotProduct(Hit.ImpactNormal, -SideDir) >= C.MinFacing;

	if (bValid)
	{
		OutNormal = Hit.ImpactNormal;
		OutTarget = Hit.ImpactPoint + Hit.ImpactNormal * C.PalmOffset;
		const float Dist = FVector::Dist(OutTarget, Shoulder);
		bValid = Dist >= C.MinShoulderDistance && Dist <= ArmLength[Arm] * C.MaxReachRatio;
	}

	DrawWallTouchProbe(Start, End, bHit, bValid, Hit.ImpactPoint);
	return bValid;
}

FVector UIkContextComponent::GetUpperArmLocation(int32 Side) const
{
	// Real bone from the last evaluated pose: it already includes the shoulder avoidance twist.
	const int32 Index = UpperArmIndex[ArmIndex(Side)];
	return Index != INDEX_NONE
		? OwnerCharacter->GetMesh()->GetBoneTransform(Index).GetLocation()
		: Capsule->GetComponentLocation();
}

void UIkContextComponent::WriteWallTouchChannels()
{
	// Single writer of the arm channels. When the wall brace comes, the arbitration goes here.
	LocoComp->ArmL.Reset();
	LocoComp->ArmR.Reset();

	const FWallTouchRuntime& S = WallTouchState;
	if (S.Side == 0 || (S.PoseAlpha <= 0.f && S.IKAlpha <= 0.f)) return;

	FIkArmChannel& Arm = S.Side > 0 ? LocoComp->ArmR : LocoComp->ArmL;
	Arm.Pose           = EIkArmPose::WallTouch;
	Arm.PoseAlpha      = S.PoseAlpha;
	Arm.IKAlpha        = S.IKAlpha;
	Arm.TargetLocation = S.Target;
	Arm.TargetRotation = FRotationMatrix::MakeFromXZ(-S.WallNormal, FVector::UpVector).Rotator();

	// Elbow pole: below, behind and slightly toward the wall, so the elbow never flips up.
	Arm.JointTargetLocation = GetUpperArmLocation(S.Side)
		- FVector::UpVector * WallTouch.ElbowPoleDown
		- S.Forward * WallTouch.ElbowPoleBack
		- S.WallNormal * WallTouch.ElbowPoleOut;
}

void UIkContextComponent::DrawWallTouchProbe(const FVector& Start, const FVector& End, bool bHit, bool bValid, const FVector& HitPoint) const
{
#if ENABLE_DRAW_DEBUG
	if (!CVarDebugWallTouch.GetValueOnGameThread()) return;

	// Green: free. Yellow: hit, but not a valid wall or out of reach. Red: valid wall.
	const FColor Color = bValid ? FColor::Red : bHit ? FColor::Yellow : FColor::Green;
	DrawDebugLine(GetWorld(), Start, End, Color, false, 0.f, 0, 0.5f);
	DrawDebugSphere(GetWorld(), Start, WallTouch.ProbeRadius, 8, Color, false, 0.f, 0, 0.3f);
	if (bHit)
		DrawDebugPoint(GetWorld(), HitPoint, 10.f, Color, false, 0.f);
#endif
}

void UIkContextComponent::DebugWallTouch() const
{
#if ENABLE_DRAW_DEBUG
	if (!CVarDebugWallTouch.GetValueOnGameThread()) return;

	const FWallTouchRuntime& S = WallTouchState;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(42, 0.f, FColor::Cyan, FString::Printf(
			TEXT("WallTouch [%s]  side %d   t %.2f / %.2f   Pose %.2f   IK %.2f"),
			*UEnum::GetDisplayValueAsText(S.State).ToString(), S.Side, S.StateTime, S.Duration, S.PoseAlpha, S.IKAlpha));

	if (S.Side == 0 || (S.PoseAlpha <= 0.f && S.IKAlpha <= 0.f)) return;

	// Hand target (cyan, with the wall normal) and elbow pole (white).
	const FIkArmChannel& Arm = S.Side > 0 ? LocoComp->ArmR : LocoComp->ArmL;
	DrawDebugSphere(GetWorld(), Arm.TargetLocation, 5.f, 8, FColor::Cyan, false, 0.f, 0, 0.5f);
	DrawDebugLine(GetWorld(), Arm.TargetLocation, Arm.TargetLocation + S.WallNormal * 20.f, FColor::Cyan, false, 0.f, 0, 0.5f);
	DrawDebugSphere(GetWorld(), Arm.JointTargetLocation, 4.f, 6, FColor::White, false, 0.f, 0, 0.3f);
#endif
}
#pragma endregion 