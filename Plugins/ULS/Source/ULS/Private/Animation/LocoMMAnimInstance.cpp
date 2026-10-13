// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LocoMMAnimInstance.h"
#include "Components/LocomotionStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ULS.h"

void ULocoMMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnsureLocoComp();
	
	// first set of FootPlacement variables
	PlantSettings = PlantSettingsDefault;
	InterpolationSettings = InterpolationSettingsDefault;
}

void ULocoMMAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnsureLocoComp()) return;

	PullFromComponent();	// stato aggiornato -> membri letti da chooser e nodo MM
	UpdateSelectionConditions(DeltaSeconds);
	SetFootPlacement();	
	UpdateAlphas(DeltaSeconds);
}

float ULocoMMAnimInstance::GetTrajectoryTurnAngle() const
{
	return ( TrjFutureVelocity.Rotation() - Velocity.Rotation() ).GetNormalized().Yaw;
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
	LocomotionMode		 = LocoComp->LocomotionMode;

	OrientationDirection = LocoComp->OrientationDirection;
	LeanAngle            = LocoComp->LeanAngleMM;
	
	PlayRate             = LocoComp->PlayRate;
	
	Trajectory           = LocoComp->Trajectory;
	TrjPastVelocity      = LocoComp->TrjPastVelocity;
	TrjCurrentVelocity	 = LocoComp->TrjCurrentVelocity;
	TrjFutureVelocity    = LocoComp->TrjFutureVelocity;
	if (!Velocity.IsNearlyZero()) LastNonZeroVelocity = Velocity;
	
}

void ULocoMMAnimInstance::UpdateSelectionConditions(float DeltaTime)
{
	/** 1) IsMoving */
	bIsMoving = !TrjFutureVelocity.Equals(FVector::ZeroVector, 10.f) && !Acceleration.IsZero();
	
	/** 2) IsStarting */
	static const FName PivotsTags(TEXT("Pivots"));
	bIsStarting = bIsMoving 
		&& TrjFutureVelocity.Size2D() >= Velocity.Size2D() + 100.f
		&& !CurrentDatabaseTags.Contains(PivotsTags);
	
	/** 3) IsPivoting */
	bIsPivoting = bIsMoving && FMath::Abs(GetTrajectoryTurnAngle()) >= PivotAngleThreshold;
	
	/** 4) ShouldTurnInPlace */
	if (const USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
	{
		const float RootYaw = FRotator::NormalizeAxis(Mesh->GetBoneTransform(0, FTransform::Identity).Rotator().Yaw);
		bShouldTurnInPlace = FMath::Abs(RootYaw) >= TurnInPlaceAngleThreshold;
	}
	
	/** 5) JustLanded */
	if (PrevLocomotionMode == ELocomotionMode::InAir && LocomotionMode == ELocomotionMode::Ground)
		TimeSinceLanded = 0.f;
	else
		TimeSinceLanded += DeltaTime;
	
	bJustLanded = TimeSinceLanded < JustLandedWindow;
	
	/** o ti stai fermando o hai cambiato gait in movimento */
	bDatabaseCategoryChanged = LocomotionMode != PrevLocomotionMode
		|| ( LocomotionMode == ELocomotionMode::Ground
			&& ( bIsMoving != bPrevIsMoving 
				|| bIsCrouched != bPrevIsCrouched 
				|| (bIsMoving && MovementGait != PrevGait) ) ); 
	
	bPrevIsMoving = bIsMoving;
	bPrevIsCrouched = bIsCrouched;
	PrevGait = MovementGait;
	PrevLocomotionMode = LocomotionMode;
}

void ULocoMMAnimInstance::SetFootPlacement()
{
	static const FName StopsTag(TEXT("Stops"));
	const bool bStops = CurrentDatabaseTags.Contains(StopsTag);
	PlantSettings         = bStops ? PlantSettingsStops         : PlantSettingsDefault;
	InterpolationSettings = bStops ? InterpolationSettingsStops : InterpolationSettingsDefault;
}

void ULocoMMAnimInstance::UpdateAlphas(float DeltaTime)
{
	const float GroundAlpha = LocomotionMode == ELocomotionMode::InAir ? 0.f : 1.f;
	TargetFootPlacementAlpha = GroundAlpha;
	TargetLegIkAlpha = GroundAlpha;
	
	FootPlacementAlpha = FMath::FInterpTo(FootPlacementAlpha, TargetFootPlacementAlpha, DeltaTime, FootPlacementAlphaSpeed);
	LegIkAlpha = FMath::FInterpTo(LegIkAlpha, TargetLegIkAlpha, DeltaTime, LegIkAlphaSpeed);
}
