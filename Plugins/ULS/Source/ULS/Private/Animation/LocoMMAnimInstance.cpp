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
	UpdateOffsetRoot();
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
	
	LandType			 = LocoComp->LandType;
	
	OrientationDirection = LocoComp->OrientationDirection;
	LeanAngle            = LocoComp->LeanAngleMM;
	
	PlayRate             = LocoComp->PlayRate;

	// Li scrive l'host dopo la sua valutazione: lo Slot e le curve dei montaggi li vede solo lui.
	bMontageActive            = LocoComp->bMontageActive;
	MovingTraversalCurveValue = LocoComp->MovingTraversalCurveValue;
	bTraversalEndsOnTop = LocoComp->bTraversalEndsOnTop;
	
	Trajectory           = LocoComp->Trajectory;
	TrjPastVelocity      = LocoComp->TrjPastVelocity;
	TrjCurrentVelocity	 = LocoComp->TrjCurrentVelocity;
	TrjFutureVelocity    = LocoComp->TrjFutureVelocity;
	if (!Velocity.IsNearlyZero()) LastNonZeroVelocity = Velocity;
	
}

void ULocoMMAnimInstance::UpdateSelectionConditions(float DeltaTime)
{
	// Traversal da fermi: mantle e climb senza curva MovingTraversal. Finiscono in piedi sopra l'ostacolo,
	// quindi sotto il montaggio il MM segue solo gli idle. Hurdle, vault e mantle in corsa seguono il movimento.
	const bool bStandingTraversal = LocomotionMode == ELocomotionMode::Traversal
		&& bTraversalEndsOnTop && MovingTraversalCurveValue <= 0.5f;
	if (bStandingTraversal) Speed2D = 0.f;
	
	/** 1) IsMoving */
	bIsMoving = !bStandingTraversal && !TrjFutureVelocity.Equals(FVector::ZeroVector, 10.f) && !Acceleration.IsZero();
	
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

	/**
	 * 6) JustTraversed: come JustLanded, una finestra che si apre quando si esce dal traversal a terra.
	 * Vale solo se il traversal era fatto in movimento. Lo dice la curva MovingTraversal del montaggio:
	 * se ne tiene il massimo visto durante il traversal, perché a montaggio finito la curva torna a zero.
	 * La soglia a 0.5 ignora i residui di uno sfumato precedente.
	 */
	if (LocomotionMode == ELocomotionMode::Traversal)
	{
		if (PrevLocomotionMode != ELocomotionMode::Traversal) TraversalCurveMax = 0.f;	// traversal nuovo
		TraversalCurveMax = FMath::Max(TraversalCurveMax, MovingTraversalCurveValue);
	}

	if (PrevLocomotionMode == ELocomotionMode::Traversal && LocomotionMode == ELocomotionMode::Ground)
	{
		TimeSinceTraversed = 0.f;
		bTraversalWasMoving = TraversalCurveMax > 0.5f;
	}
	else
		TimeSinceTraversed += DeltaTime;

	bJustTraversed = bTraversalWasMoving
		&& LocomotionMode == ELocomotionMode::Ground
		&& TimeSinceTraversed < JustTraversedWindow
		&& FMath::Abs(GetTrajectoryTurnAngle()) <= JustTraversedMaxTurnAngle;
	
	/**
	 * o ti stai fermando o hai cambiato gait in movimento.
	 * L'uscita dal traversal è un cambio di LocomotionMode: l'interruzione cade nello stesso frame
	 * in cui si apre JustTraversed, quindi la ricerca dopo il traversal vede già FromTraversal.
	 */
	bDatabaseCategoryChanged = LocomotionMode != PrevLocomotionMode
		|| ( LocomotionMode == ELocomotionMode::Ground
			&& ( bIsMoving != bPrevIsMoving 
				|| bIsCrouched != bPrevIsCrouched 
				|| (bIsMoving && MovementGait != PrevGait) ) ); 
	
	// Uscita da un mantle fatto da fermi: il MM cerca da capo sulla posa finale del montaggio, con un blend corto.
	// Senza, continuava il clip seguito sotto il montaggio, che resta tra gli idle e a volte è una posa più bassa.
	const bool bLeftStandingMantle = PrevLocomotionMode == ELocomotionMode::Traversal
		&& LocomotionMode != ELocomotionMode::Traversal
		&& bTraversalEndsOnTop && !bTraversalWasMoving;

	MMInterruptMode = bLeftStandingMantle ? EPoseSearchInterruptMode::ForceInterrupt
		: bDatabaseCategoryChanged ? EPoseSearchInterruptMode::InterruptOnDatabaseChange
		: EPoseSearchInterruptMode::DoNotInterrupt;
	MMBlendTime = bLeftStandingMantle ? MantleExitBlendTime : DefaultMMBlendTime;
	
	bPrevIsMoving = bIsMoving;
	bPrevIsCrouched = bIsCrouched;
	PrevGait = MovementGait;
	PrevLocomotionMode = LocomotionMode;
}

void ULocoMMAnimInstance::UpdateOffsetRoot()
{
	// Con un montaggio nello Slot dell'host (traversal, roll, slide) l'offset si rilascia:
	// la radice del layer segue la capsula e, quando il montaggio sfuma, non c'è niente da recuperare.
	// Le vecchie funzioni Blueprint lo chiedevano a IsSlotActive, ma lo Slot sta nell'host e il layer non lo vede.
	if (bMontageActive)
	{
		OffsetRootTranslationMode = EOffsetRootBoneMode::Release;
		OffsetRootRotationMode    = EOffsetRootBoneMode::Release;
	}
	else
	{
		OffsetRootTranslationMode = bShouldMove ? EOffsetRootBoneMode::Interpolate : EOffsetRootBoneMode::Release;
		OffsetRootRotationMode    = EOffsetRootBoneMode::Accumulate;
	}

	OffsetRootTranslationHalfLife = bShouldMove ? OffsetRootHalfLifeMoving : OffsetRootHalfLifeIdle;
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
	const bool bNotActive = LocomotionMode == ELocomotionMode::InAir || LocomotionMode == ELocomotionMode::Slide || LocomotionMode == ELocomotionMode::Traversal;
	const float GroundAlpha = bNotActive ? 0.f : 1.f;
	TargetFootPlacementAlpha = GroundAlpha;
	TargetLegIkAlpha = GroundAlpha;
	
	
	//FootPlacementAlpha = FMath::FInterpTo(FootPlacementAlpha, TargetFootPlacementAlpha, DeltaTime, FootPlacementAlphaSpeed);
	//LegIkAlpha = FMath::FInterpTo(LegIkAlpha, TargetLegIkAlpha, DeltaTime, LegIkAlphaSpeed);
}