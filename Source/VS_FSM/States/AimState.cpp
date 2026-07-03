// Fill out your copyright notice in the Description page of Project Settings.


#include "States/AimState.h"

#include "CameraModeDataAsset.h"

void UAimState::OnJump()
{
	Super::OnJump();
	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, "Jumping");
}

void UAimState::OnCrouch()
{
	Super::OnCrouch();
	AnimInstance->bIsCrouched = !AnimInstance->bIsCrouched;
	if (CameraRef)
		CameraRef->SetCameraMode(AnimInstance->bIsCrouched ? AimCrouchCameraData : AimCameraData);
	
	PushYawCorrection();
}

void UAimState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	AnimInstance->LastRootYawOffset = 0.f;
	AnimInstance->RootYawMode = ERootYawMode::Accumulate;
	AnimInstance->bShouldTurnLeft = false;
	AnimInstance->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	// Camera
	if (CameraRef)
		CameraRef->SetCameraMode(AnimInstance->bIsCrouched ? AimCrouchCameraData : AimCameraData);
	
	PushYawCorrection();
	
	AnimInstance->bIsJogging = false;
}

void UAimState::OnExitState()
{
	Super::OnExitState();

}

void UAimState::PushYawCorrection() const
{
	const UCameraModeDataAsset* CamDA = AnimInstance->bIsCrouched ? AimCrouchCameraData : AimCameraData;
	if (!CamDA) return;
	
	const float LateralOffset = CamDA->SocketOffset.Y + CamDA->TargetSocket.Y;
	
	AnimInstance->AimYawCorrection = FMath::RadiansToDegrees(FMath::Atan2(LateralOffset, ConvergenceDistance));
}

void UAimState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!AnimInstance->bIsAiming)
	{
		if (AnimInstance->bIsCrouched)
		{
			if (!PlayerRef->IsMoving()) 
			{
				PlayerRef->StateManager->SwitchStateByKey("Crouch_Idle");
				return;
			}
			
			PlayerRef->StateManager->SwitchStateByKey("Crouch_Walk");
			return;
		}
		
		if (!PlayerRef->IsMoving())
		{
			PlayerRef->StateManager->SwitchStateByKey("Idle");
			return;
		}
		
		PlayerRef->StateManager->SwitchStateByKey("Walk");
		return;
	}
#pragma endregion
	
	if (FMath::Abs(AnimInstance->RootYawOffset) > 0.1f)
	{
		AnimInstance->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			AnimInstance->RootYawOffset, 0.f, SpringState,
			120.f, 1.f, DeltaTime);
	}
	else
	{
		AnimInstance->RootYawOffset = 0.f;
	}
	
	
	if (AnimInstance->bShouldMove || PlayerRef->GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
		UpdateOrientationDirection(DeltaTime);
	
	UpdateAnimationParameters(DeltaTime);
	
	if (AnimInstance->FinalAimPose)
	{
		const float Pitch = FRotator::NormalizeAxis(PlayerRef->GetControlRotation().Pitch);
		const float Norm = FMath::GetMappedRangeValueClamped(
			FVector2D(-SweepPitchRange, SweepPitchRange), FVector2D(0.f, 1.f), Pitch);
		AnimInstance->AimPoseTime = Norm * AnimInstance->FinalAimPose->GetPlayLength();
	}
	
#pragma region DEBUG
#if ENABLE_DRAW_DEBUG
	if (const USkeletalMeshComponent* Mesh = PlayerRef->GetMesh())
	{
		const FVector Start = Mesh->GetSocketLocation(TEXT("WP_WolverineSocket"));
		const FVector Dir = PlayerRef->GetControlRotation().Vector();
		DrawDebugLine(GetWorld(), Start, Start + Dir * 2000.f, FColor::Red, false, -1.f, 0, 0.5f);
	}
#endif
	GEngine->AddOnScreenDebugMessage(80, 0.f, FColor::Orange,
	FString::Printf(TEXT("AimYawCorrection: %.2f | AimAlpha: %.2f"), AnimInstance->AimYawCorrection, AnimInstance->AimAlpha));
#pragma endregion
}

/* 
 *  Ricorda nei enter condition di mettere eventualmente i check se sta nei vari stop state in graph */