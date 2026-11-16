// Fill out your copyright notice in the Description page of Project Settings.


#include "States/PipeState.h"

#include "Components/CapsuleComponent.h"
#include "Data/PipeDataAsset.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayActors/PipeActor.h"

void UPipeState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);
	MeshBaseScale = PlayerRef->GetMesh()->GetRelativeScale3D();
	BreathTime = 0.f;
	BreathValue = 0.f;
	
	Pipe = LocoComp->PendingPipe;
	PipeDataAsset = Cast<UPipeDataAsset>(StateData);
	
	if (!Pipe || !PipeDataAsset) return;
	
	LocoComp->LocomotionMode = ELocomotionMode::Pipe;
	bExitCrouched = LocoComp->bIsCrouched;
	LocoComp->bIsCrouched = false;
	LocoComp->ResetFSMTransientState();
	
	if (AController* PC = PlayerRef->GetController()) PC->SetIgnoreMoveInput(true);
	CharacterMovementComponent->StopMovementImmediately();
	CharacterMovementComponent->SetMovementMode(MOVE_Flying);
	// RICORDA DA RotationRate = 0;
	
	bFromTop = LocoComp->bPendingPipeFromTop;

	StepCount = 0;
	Step = RootMotionZ(PipeDataAsset->ClimbUpLeft);
	ExitZ = Pipe->GetTopZ() - RootMotionZ(PipeDataAsset->ExitTop);

	// Punto di presa finale, ai piedi.
	EnterGrip = Pipe->GetGripTransform(PipeDataAsset->GripDistance, 0.f);
	const float GripYaw = EnterGrip.Rotator().Yaw;

	// Dove devono stare piedi e rotazione quando parte la clip.
	FVector StartFeet = EnterGrip.GetLocation();
	float StartYaw = GripYaw;

	if (!bFromTop)
	{
		EnterMontage = PipeDataAsset->EnterBottom;
		EntryZ = Pipe->GetBaseZ();
		StartFeet.Z = EntryZ;
		bRightHand = true;
	}
	else
	{
		EnterMontage = PipeDataAsset->EnterTop;
		EntryZ = Pipe->GetTopZ() - RootMotionZ(EnterMontage);	// la quota dove ti lascia la clip
		bRightHand = false;
		
		if (EnterMontage)
		{
			const float StandHalfHeight = PlayerRef->GetDefaultHalfHeight();

			// Dove la clip deve finire: in presa, alla quota EntryZ.
			const FTransform End(FRotator(0.f, GripYaw, 0.f),
				FVector(EnterGrip.GetLocation().X, EnterGrip.GetLocation().Y, EntryZ + StandHalfHeight));

			// Root motion totale della clip, portato dallo spazio della mesh a quello dell'attore.
			const FTransform MeshRel = PlayerRef->GetMesh()->GetRelativeTransform();
			const FTransform RootMotion = EnterMontage->ExtractRootMotionFromTrackRange(
				0.f, EnterMontage->GetPlayLength(), FAnimExtractContext());
			const FTransform ActorDelta = MeshRel.Inverse() * RootMotion * MeshRel;

			// Da dove deve partire perché finisca lì.
			const FTransform Start = ActorDelta.Inverse() * End;
			StartFeet = Start.GetLocation() - FVector(0.f, 0.f, StandHalfHeight);
			StartYaw = Start.Rotator().Yaw;
		}
	}

	const FVector PlayerLocation = PlayerRef->GetActorLocation();
	const float FeetZ = PlayerLocation.Z - PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	SnapDeltaLoc = FVector(StartFeet.X - PlayerLocation.X, StartFeet.Y - PlayerLocation.Y, StartFeet.Z - FeetZ);
	SnapDeltaYaw = FMath::FindDeltaAngleDegrees(PlayerRef->GetActorRotation().Yaw, StartYaw);
	PrevAlpha = 0.f;

	// Dal basso la clip parte subito. Dall'alto parte a fine snap: lo fa il tick.
	bEnterStarted = !bFromTop;
	if (bEnterStarted) PlayMontage(EnterMontage);

	Phase = EPipePhase::Entering;
	PhaseTime = 0.f;
}

void UPipeState::OnExitState()
{
	if (CurrentMontage != nullptr)
	{
		PlayerRef->GetMesh()->GetAnimInstance()->Montage_Stop(0.2, CurrentMontage);
		CurrentMontage = nullptr;
	}
	
	if (AController* PC = PlayerRef->GetController()) PC->SetIgnoreMoveInput(false);
	
	SetWorldCollision(true);
	if (CharacterMovementComponent->MovementMode == MOVE_Flying)	// se siamo in FALLING rimanici
		CharacterMovementComponent->SetMovementMode(MOVE_Walking);
	Pipe = nullptr;
	
	PlayerRef->GetMesh()->SetRelativeScale3D(MeshBaseScale);
	Super::OnExitState();
}

void UPipeState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	if (!Pipe || !PipeDataAsset)
	{
		StateManager->SwitchStateByKey("Idle");
		return;
	}
	UpdateAnimationParameters(DeltaTime);
	
	PhaseTime += DeltaTime;

	switch (Phase)
	{
		case EPipePhase::Entering:
		{
			const float SnapTime = bFromTop ? PipeDataAsset->EnterTopSnapTime : PipeDataAsset->EnterSnapTime;
			const float A = FMath::SmoothStep(0.f, 1.f, PhaseTime / FMath::Max(SnapTime, 0.01f));
			const float D = A - PrevAlpha;
			PrevAlpha = A;

			PlayerRef->AddActorWorldOffset(SnapDeltaLoc * D);
			PlayerRef->AddActorWorldRotation(FRotator(0.f, SnapDeltaYaw * D, 0.f));

			// Dall'alto la clip parte solo quando il player è esattamente sul punto di partenza.
			if (A >= 1.f && !bEnterStarted)
			{
				SetWorldCollision(false);	// scendendo la capsula attraversa lo spigolo
				PlayMontage(EnterMontage);
				bEnterStarted = true;
			}

			if (A >= 1.f && bEnterStarted && IsAtEnd(EnterMontage))
			{
				PlayerRef->SetActorLocationAndRotation(EnterGrip.GetLocation(), FRotator(0.f, EnterGrip.Rotator().Yaw, 0.f));
				SetRootZ(EntryZ);
				CharacterMovementComponent->StopMovementImmediately();
				SetWorldCollision(true);
				Phase = EPipePhase::Hanging;
			}
			break;
		}
		
		case EPipePhase::Hanging:
		{
			CharacterMovementComponent->Velocity = FVector::ZeroVector;
			SetRootZ(EntryZ + StepCount * Step);

			const float Y = LocoComp->MoveInput.Y;

				if (Y > PipeDataAsset->InputThreshold)
			{
				if (RootZ() >= ExitZ - 1.f) StartTopExit();		// già alla quota d'uscita
				else StartStroke(true);
			}
			else if (Y < -PipeDataAsset->InputThreshold && RootZ() - Step >= Pipe->GetBaseZ() - 1.f)
			{
				StartStroke(false);
			}
			break;
		}

		case EPipePhase::Climbing:
		{
			// In cima: taglio la bracciata alla quota d'uscita e parte ExitTop.
			if (bGoingUp && RootZ() >= ExitZ)
			{
				SetRootZ(ExitZ);
				StartTopExit();
				break;
			}

			if (IsAtEnd(CurrentMontage))
			{
				CharacterMovementComponent->StopMovementImmediately();
				StepCount += bGoingUp ? 1 : -1;
				SetRootZ(EntryZ + StepCount * Step);
				bRightHand = !bRightHand;
				Phase = EPipePhase::Hanging;
			}
			break;
		}

		case EPipePhase::Exiting:
			break;	// aspetta OnExitMontageBlendingOut
		
	}
	UpdateBreath(DeltaTime);
}

void UPipeState::OnCrouch()
{
	if (Phase == EPipePhase::Entering || Phase == EPipePhase::Exiting) return;
	
	LocoComp->bBlockPipeAttach = true;
	
	if (RootZ() <= Pipe->GetBaseZ() + 1)	// base level
	{
		ExitToGround();
		return;
	}
	if (PipeDataAsset->DetachPush > 0)
		CharacterMovementComponent->Velocity = -Pipe->GetClimbFacing() * PipeDataAsset->DetachPush;
	
	CharacterMovementComponent->SetMovementMode(MOVE_Falling);
}


// ---- HELPERS ----

void UPipeState::PlayMontage(UAnimMontage* AnimMontage)
{
	if (AnimMontage == nullptr) return;
	
	PlayerRef->GetMesh()->GetAnimInstance()->Montage_Play(AnimMontage);
	CurrentMontage = AnimMontage;
}

bool UPipeState::IsAtEnd(UAnimMontage* AnimMontage)
{
	if (AnimMontage == nullptr) return true;
	const UAnimInstance* AnimInstance = PlayerRef->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return true;
	
	if (!AnimInstance->Montage_IsActive(AnimMontage)) return true;
	
	return AnimInstance->Montage_GetPosition(AnimMontage) >= AnimMontage->GetPlayLength() - 0.01f;
}

void UPipeState::SetRootZ(float Z)
{
	const FVector PlayerLoc = PlayerRef->GetTransform().GetLocation();
	PlayerRef->SetActorLocation(FVector(PlayerLoc.X, PlayerLoc.Y, Z + PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
}

float UPipeState::RootZ() const
{
	return PlayerRef->GetActorLocation().Z - PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void UPipeState::ExitToGround()
{
	StateManager->SwitchStateByKey(bExitCrouched ? "Crouch_Idle" : "Idle");
}

float UPipeState::RootMotionZ(UAnimMontage* AnimMontage) const
{
	if (AnimMontage == nullptr) return 0.0f;	
	return FMath::Abs(AnimMontage->ExtractRootMotionFromTrackRange(0, AnimMontage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Z);
}

void UPipeState::StartStroke(bool bUp)
{
	UAnimMontage* M = bUp ? (bRightHand ? PipeDataAsset->ClimbUpLeft : PipeDataAsset->ClimbUpRight)
						: (bRightHand ? PipeDataAsset->ClimbDownRight : PipeDataAsset->ClimbDownLeft);
	
	PlayMontage(M);
	bGoingUp = bUp;
	Phase = EPipePhase::Climbing;
}

void UPipeState::UpdateBreath(float DeltaTime)
{
	float Target = 0.f;
	if (Phase == EPipePhase::Hanging)
	{
		BreathTime += DeltaTime;
		const float Period = FMath::Max(PipeDataAsset->BreathPeriod, 0.1f);
		Target = 0.5f * (1.f - FMath::Cos(2.f * PI * BreathTime / Period));	// 0..1, parte da 0
	}
	else
	{
		BreathTime = 0.f;	// fuori da Hanging il respiro rientra
	}

	BreathValue = FMath::FInterpTo(BreathValue, Target, DeltaTime, 8.f);

	FVector Scale = MeshBaseScale;
	Scale.Z *= 1.f + PipeDataAsset->BreathAmount * BreathValue;
	PlayerRef->GetMesh()->SetRelativeScale3D(Scale);
}

void UPipeState::SetWorldCollision(bool bOn)
{
	UCapsuleComponent* Capsule = PlayerRef->GetCapsuleComponent();
	const ECollisionResponse Response = bOn ? ECR_Block : ECR_Ignore;
	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, Response);
	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, Response);
}

void UPipeState::StartTopExit()
{
	CharacterMovementComponent->StopMovementImmediately();

	UAnimMontage* ExitMontage = PipeDataAsset->ExitTop;
	UAnimInstance* AnimInstance = PlayerRef->GetMesh()->GetAnimInstance();
	if (!ExitMontage || !AnimInstance)
	{
		Phase = EPipePhase::Hanging;	// niente clip d'uscita: resto appeso
		return;
	}

	SetWorldCollision(false);	// la capsula attraversa lo spigolo del muro
	PlayMontage(ExitMontage);

	FOnMontageBlendingOutStarted BlendingOut;
	BlendingOut.BindUObject(this, &UPipeState::OnExitMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOut, ExitMontage);

	Phase = EPipePhase::Exiting;
}

void UPipeState::OnExitMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// Interrotto vuol dire che l'ha fermato OnExitState: non c'è niente da fare.
	if (bInterrupted || Phase != EPipePhase::Exiting || !Pipe) return;

	CurrentMontage = nullptr;				// sta già sfumando da solo
	SetRootZ(Pipe->GetTopZ() + 2.f);		// piedi un filo sopra il pavimento, mai dentro
	ExitToGround();
}
