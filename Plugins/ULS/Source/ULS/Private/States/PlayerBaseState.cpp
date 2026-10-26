// Fill out your copyright notice in the Description page of Project Settings.


#include "States/PlayerBaseState.h"
#include "Components/VSCharacterMovementComponent.h"
#include "Data/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TraversalComponent.h"
#include "Components/CapsuleComponent.h"

bool UPlayerBaseState::IsMoving() const
{
	return PlayerRef && PlayerRef->GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER;
}

void UPlayerBaseState::OnEnterState(AActor* OwnerRef)
{	
	//Save player ref for later
	if (!PlayerRef)
		PlayerRef = Cast<ACharacter>(OwnerRef);
	
	if (!StateManager && PlayerRef)
		StateManager = PlayerRef->FindComponentByClass<UStateManagerComponent>();
	
	//Save CMC
	if (!CharacterMovementComponent && PlayerRef)
		CharacterMovementComponent = PlayerRef->GetCharacterMovement();
	
	if (CharacterMovementComponent)
		CharacterMovementComponent->bWantsToCrouch = WantsCrouchedCapsule();
	
	if (!LocoComp && PlayerRef)
		LocoComp = PlayerRef->FindComponentByClass<ULocomotionStateComponent>();
	
	//Save CameraComponent
	if (!CameraRef && PlayerRef)
		CameraRef = Cast<UVSCameraComponent>(PlayerRef->FindComponentByClass<UVSCameraComponent>());
	
	// Memoria del lean: parte dai valori di adesso, altrimenti il primo frame
	// dello stato misura la differenza con l'ultima volta che ci eri stato.
	if (PlayerRef)
	{
		PreviousVelocity = PlayerRef->GetVelocity();
		PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	}
	
	//Bind Delegates
	SetupDelegates();
	
	ApplyMovementParameters();
}

void UPlayerBaseState::OnExitState()
{
	//Super::OnExitState();		--> empty
	
	ResetDelegates();
}

void UPlayerBaseState::SetupDelegates()
{
	if (LocoComp) LocoComp->StanceChangedDelegate.AddUObject(this, &UPlayerBaseState::ApplyMovementParameters);
	if (LocoComp) LocoComp->CrouchRequestedDelegate.AddUObject(this, &UPlayerBaseState::OnCrouch);
	if (LocoComp) LocoComp->JumpRequestedDelegate.AddUObject(this, &UPlayerBaseState::OnJump);
	if (PlayerRef)
	{
		PlayerRef->MovementModeChangedDelegate.AddUniqueDynamic(this, &UPlayerBaseState::HandleMovementModeChanged);
		PlayerRef->LandedDelegate.AddUniqueDynamic(this, &UPlayerBaseState::HandleLanded);
	}
}

void UPlayerBaseState::ResetDelegates()
{
	if (LocoComp) LocoComp->StanceChangedDelegate.RemoveAll(this);
	if (LocoComp) LocoComp->CrouchRequestedDelegate.RemoveAll(this);
	if (LocoComp) LocoComp->JumpRequestedDelegate.RemoveAll(this);
	if (PlayerRef)
	{
		PlayerRef->MovementModeChangedDelegate.RemoveDynamic(this, &UPlayerBaseState::HandleMovementModeChanged);
		PlayerRef->LandedDelegate.RemoveDynamic(this, &UPlayerBaseState::HandleLanded);
	}
}

void UPlayerBaseState::ApplyMovementParameters()
{
	StateData = ResolveStateData();
	
	if (!IsValid(StateData) || !CharacterMovementComponent || !LocoComp)  return;
	
	//Import State Data and Refresh in Custom Anim Instance
	CharacterMovementComponent->MaxWalkSpeed = StateData->MovementSpeed;
	CharacterMovementComponent->MaxWalkSpeedCrouched = StateData->MovementSpeed;
	CharacterMovementComponent->MaxAcceleration = LocoComp->IsFSMBackend() ? StateData->MaxAcceleration : StateData->MaxAccelerationMM;
	CharacterMovementComponent->BrakingDecelerationWalking = LocoComp->IsFSMBackend() ? StateData->BrakingDeceleration : StateData->BrakingDecelerationMM;
	CharacterMovementComponent->BrakingFrictionFactor = StateData->BrakingFrictionFactor;
	CharacterMovementComponent->BrakingFriction = StateData->BrakingFriction;
	CharacterMovementComponent->bUseSeparateBrakingFriction = StateData->bUseSeparateBrakingFriction;
	CharacterMovementComponent->RotationRate = FRotator(0.f, StateData->RotationRate, 0.f);
		
	LocoComp->LeanStateIndex = StateData->LeanStateIndex;
	LocoComp->TargetPlayRate = StateData->PlayRate;
	LocoComp->RefreshMovementCache();
	
	
	if (UVSCharacterMovementComponent* VSMove = Cast<UVSCharacterMovementComponent>(CharacterMovementComponent))
	{
		VSMove->LateralScale  = LocoComp->IsFSMBackend() ? StateData->LateralSpeedScale : StateData->LateralSpeedScaleMM;
		VSMove->BackwardScale = LocoComp->IsFSMBackend() ? StateData->BackwardSpeedScale : StateData->BackwardSpeedScaleMM;
	}
}


/* ---> COMUNE A TUTTI GLI STATI CHE MUOVONO IL PERSONAGGIO
 * Orientamento, velocity e lean. Prima stavano in ULocomotionState:
 * qui girano anche in InAir, Slide e Landing.
**/

FVector UPlayerBaseState::GetIntendedDir()
{
	FVector Dir = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
	if (Dir.IsNearlyZero()) Dir = PlayerRef->GetPendingMovementInputVector(); // input non ancora consumato
	if (Dir.IsNearlyZero()) Dir = PlayerRef->GetVelocity();
	return Dir.GetSafeNormal2D(); // può tornare zero se davvero non c'è nulla
}

void UPlayerBaseState::PushOrientationDirection(FVector InSmoothedDir)
{	
	if (InSmoothedDir.IsNearlyZero()) return;
	
	const FVector Forward = PlayerRef->GetActorForwardVector();
	const FVector Velocity = PlayerRef->GetVelocity();
	
	// --- Segnale FILTRATO -> Decisioni
	const float DotDecision = FVector::DotProduct(Forward, InSmoothedDir);
	const float CrossZDecision = FVector::CrossProduct(Forward, InSmoothedDir).Z;
	const float DecisionAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZDecision, DotDecision));
	
	// --- Segnale GREZZO -> Correzzioni
	float WarpAngle = DecisionAngle;
	if (Velocity.Size2D() >= StateData->MinSpeedForOrientation)	// Calcoliamo WarpAngle, scartiamo il fallback.
	{
		const FVector VelDir = Velocity.GetSafeNormal2D();
		const float Dot = FVector::DotProduct(Forward, VelDir);
		const float CrossZ = FVector::CrossProduct(Forward, VelDir).Z;
		WarpAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	}
	
	// Warping
	LocoComp->Fwd   = FMath::UnwindDegrees(WarpAngle);
	LocoComp->Bwd   = FMath::UnwindDegrees(WarpAngle - 180.f);
	LocoComp->Left  = FMath::UnwindDegrees(WarpAngle + 90.f);
	LocoComp->Right = FMath::UnwindDegrees(WarpAngle - 90.f);
	
	// Bucketing
	LocoComp->OrientationAngle = DecisionAngle;
	
	const float AbsAngle = FMath::Abs(DecisionAngle);
	if (AbsAngle <= StateData->ForwardHalfAngle)   LocoComp->OrientationDirection = EOrientationDirection::Forward;
	else if (AbsAngle >= 180.f - StateData->BackwardHalfAngle)    LocoComp->OrientationDirection = EOrientationDirection::Backward;
	else if (DecisionAngle >= 0)   LocoComp->OrientationDirection = EOrientationDirection::Right;
	else LocoComp->OrientationDirection = EOrientationDirection::Left;
}

void UPlayerBaseState::UpdateOrientationDirection(float DeltaTime)		//Also Update values of direction in ABP -> Now we take accelleration, safer
{
	const FVector Velocity = PlayerRef->GetVelocity();
	FVector TargetDir;
	
	if (Velocity.Size2D() >= StateData->MinSpeedForOrientation)	
	{
		TargetDir = Velocity.GetSafeNormal2D();
	}
	else
	{
		const FVector Accel = PlayerRef->GetCharacterMovement()->GetCurrentAcceleration();
		if (Accel.IsNearlyZero()) return; // quasi a 0, scarta tutto
		TargetDir = Accel.GetSafeNormal2D();			
	}
	
	if (LocoComp->SmoothedDir.IsNearlyZero())
	{
		LocoComp->SmoothedDir = TargetDir;
		PushOrientationDirection(TargetDir);
		return;
	}
	
	const float CurrentYaw = FMath::RadiansToDegrees(FMath::Atan2(LocoComp->SmoothedDir.Y, LocoComp->SmoothedDir.X));
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(TargetDir.Y, TargetDir.X));
	
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	const float Alpha = FMath::Clamp(DeltaTime * StateData->OrientationInterpSpeed, 0.f, 1.f);
	const float NewYaw = FMath::UnwindDegrees(CurrentYaw + DeltaYaw*Alpha);
	
	LocoComp->SmoothedDir = FRotator(0,NewYaw,0).Vector();
	PushOrientationDirection(LocoComp->SmoothedDir);
}

void UPlayerBaseState::UpdateAnimationParameters(float DeltaTime)
{
	if (!PlayerRef || !LocoComp) return;
	
	// Velocity
	const FVector V = PlayerRef->GetVelocity();
	LocoComp->Velocity = V;
	LocoComp->VelocityXY = FVector(V.X, V.Y, 0.f);
	
	/** --- LEAN ANGLE SECTION --- */
	const float CurrentYaw = PlayerRef->GetActorRotation().Yaw;
	const float ActorYawDelta = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentYaw);
	PreviousActorYaw = CurrentYaw;
	
	// Yaw rate (gradi/secondo)
	const float YawRate = (DeltaTime > KINDA_SMALL_NUMBER) ? ActorYawDelta / DeltaTime : 0.f;
	
	LocoComp->LeanAngle = CalculateLeanFSM(YawRate);
	
	/** LEAN ANGLE MM*/
	const FVector MeasuredAccel = (DeltaTime > KINDA_SMALL_NUMBER) ? (V - PreviousVelocity) / DeltaTime : FVector::ZeroVector;
	PreviousVelocity = V;
	
	LocoComp->TargetLeanAngleMM = CalculateLeanMM(MeasuredAccel);
	
	LocoComp->LeanAngleMM = FMath::FInterpTo(LocoComp->LeanAngleMM, LocoComp->TargetLeanAngleMM, DeltaTime, LocoComp->LeanMMInterpSpeed);
}

void UPlayerBaseState::HandleMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode,
	uint8 PreviousCustomMode)
{
	if (CharacterMovementComponent && CharacterMovementComponent->MovementMode == MOVE_Falling)
		OnStartFalling();
}

void UPlayerBaseState::HandleLanded(const FHitResult& Hit)
{
	OnLanded(Hit);
}

void UPlayerBaseState::OnStartFalling()
{
	if (StateManager) StateManager->SwitchStateByKey("InAir");
}

const ULocomotionDataAsset* UPlayerBaseState::ResolveStateData() const
{
	const EStanceMode Stance = LocoComp ? LocoComp->StanceMode : EStanceMode::Normal;
	if (const TObjectPtr<ULocomotionDataAsset>* Found = State_StanceData.Find(Stance))
		if (*Found) return *Found;

	const TObjectPtr<ULocomotionDataAsset>* Fallback = State_StanceData.Find(EStanceMode::Normal);
	return Fallback ? *Fallback : nullptr;
}


void UPlayerBaseState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	if (LocoComp) LocoComp->bShouldMove = !LocoComp->bIsMovementInputZero;
	
#pragma region DEBUG
	
	if (const APlayerController* PC = PlayerRef->GetController<APlayerController>())
	{
		const float CtrlYaw  = PC->GetControlRotation().Yaw;
		const float CamYaw   = PC->PlayerCameraManager->GetCameraRotation().Yaw;
		const float VelYaw   = PlayerRef->GetVelocity().Rotation().Yaw;
		const float ActorYaw = PlayerRef->GetActorRotation().Yaw;

		GEngine->AddOnScreenDebugMessage(30, 0.f, FColor::Green, FString::Printf(
			TEXT("Vel-Ctrl %.1f   Cam-Ctrl %.1f   Actor-Ctrl %.1f"),
			FMath::FindDeltaAngleDegrees(CtrlYaw, VelYaw),
			FMath::FindDeltaAngleDegrees(CtrlYaw, CamYaw),
			FMath::FindDeltaAngleDegrees(CtrlYaw, ActorYaw)));
	}
	
#pragma endregion
}


// To override
void UPlayerBaseState::OnJump()
{
}

void UPlayerBaseState::OnCrouch()
{
}

bool UPlayerBaseState::TryEnterTraversal()
{
	UTraversalComponent* TraversalComp = PlayerRef ? PlayerRef->GetComponentByClass<UTraversalComponent>() : nullptr;
	if (!TraversalComp || !StateManager || TraversalComp->IsDoingTraversal()) return false;
	
	FTraversalCheckResult Result;
	if (!TraversalComp->TryTraversal(Result)) return false;
	
	StateManager->SwitchStateByKey("Traversal");
	return true;
}

bool UPlayerBaseState::CanStandUp() const
{
	const UCapsuleComponent* Capsule = PlayerRef ? PlayerRef->GetCapsuleComponent() : nullptr;
	if (!Capsule) return true;

	// Capsula in piedi con la base dove sono i piedi adesso, un filo più stretta per non toccare pavimento e pareti.
	const float StandHalfHeight = PlayerRef->GetDefaultHalfHeight();
	const FVector Loc = Capsule->GetComponentLocation();
	const float Bottom = Loc.Z - Capsule->GetScaledCapsuleHalfHeight();
	const FVector StandCenter(Loc.X, Loc.Y, Bottom + StandHalfHeight);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() - 1.f, StandHalfHeight - 2.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CanStandUp), false, PlayerRef);
	FCollisionResponseParams Response;
	Capsule->InitSweepCollisionParams(Params, Response);

	return !PlayerRef->GetWorld()->OverlapBlockingTestByChannel(
		StandCenter, FQuat::Identity, Capsule->GetCollisionObjectType(), Shape, Params, Response);
}
