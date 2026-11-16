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
	
	// Calcolo il punto di attacco
	const FTransform Grip = Pipe->GetGripTransform(PipeDataAsset->GripDistance, 0.f); // 0: punto a terra.
	EnterGrip = Grip;
	const FVector PlayerLocation = PlayerRef->GetActorLocation();
	const float FeetZ = PlayerLocation.Z - PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	SnapDeltaLoc = FVector(Grip.GetLocation().X - PlayerLocation.X, Grip.GetLocation().Y - PlayerLocation.Y, Pipe->GetBaseZ() - FeetZ);
	SnapDeltaYaw = FMath::FindDeltaAngleDegrees(PlayerRef->GetActorRotation().Yaw, Grip.Rotator().Yaw);
	PrevAlpha = 0.f;
	
	bRightHand = true;
	StepCount = 0;
	EntryZ = Pipe->GetBaseZ();
	Step = RootMotionZ(PipeDataAsset->ClimbUpLeft);
	
	PlayMontage(PipeDataAsset->EnterBottom);
	
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
			const float A = FMath::SmoothStep(0.f, 1.f, PhaseTime / PipeDataAsset->EnterSnapTime);
			const float D = A - PrevAlpha;
			PrevAlpha = A;
			
			PlayerRef->AddActorWorldOffset(SnapDeltaLoc * D);
			PlayerRef->AddActorWorldRotation(FRotator(0, SnapDeltaYaw * D, 0));
		
			if ( A >= 1 && IsAtEnd(PipeDataAsset->EnterBottom) )
			{
				PlayerRef->SetActorLocation(EnterGrip.GetLocation(), false);
				SetRootZ(Pipe->GetBaseZ());
				EnterGrip = FTransform::Identity;
				Phase = EPipePhase::Hanging;
			}
			break;
		}
		
		case EPipePhase::Hanging:
		{
			CharacterMovementComponent->Velocity = FVector::ZeroVector;
			SetRootZ(EntryZ + StepCount * Step);
				
			const float Y = LocoComp->MoveInput.Y;
				
			if (Y > PipeDataAsset->InputThreshold && RootZ() + Step <= Pipe->GetTopZ())
				StartStroke(true);
			else if (Y < -PipeDataAsset->InputThreshold && RootZ() -Step >= Pipe->GetBaseZ() - 1)
				StartStroke(false);
				
			break;
		}
		
		case EPipePhase::Climbing:
		{
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
	}
	UpdateBreath(DeltaTime);
}

void UPipeState::OnCrouch()
{
	if (Phase == EPipePhase::Entering) return;
	
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
