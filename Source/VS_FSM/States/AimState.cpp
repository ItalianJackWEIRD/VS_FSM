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
	LocoComp->bIsCrouched = !LocoComp->bIsCrouched;
	if (CameraRef)
		CameraRef->SetCameraMode(LocoComp->bIsCrouched ? AimCrouchCameraData : AimCameraData);
	
	LocoComp->FinalIdleAnim = LocoComp->bIsCrouched ? LocoComp->IdleAnims.L_02 : LocoComp->IdleAnims.R_01;
	LocoComp->FinalStanceTransitionAnim = LocoComp->bIsCrouched ? LocoComp->StanceTransitionAnims.L_02 : LocoComp->StanceTransitionAnims.R_01;
	PushYawCorrection();
}

void UAimState::OnEnterState(AActor* StateOwner)
{
	Super::OnEnterState(StateOwner);
	// Reset turn-in-place state che potrebbe essere "in volo" dall'Idle
	LocoComp->LastRootYawOffset = 0.f;
	LocoComp->RootYawMode = ERootYawMode::Accumulate;
	LocoComp->bShouldTurnLeft = false;
	LocoComp->bShouldTurnRight = false;
	
	PreviousActorYaw = PlayerRef->GetActorRotation().Yaw;
	
	// Camera
	if (CameraRef)
		CameraRef->SetCameraMode(LocoComp->bIsCrouched ? AimCrouchCameraData : AimCameraData);
	
	PushYawCorrection();
	
	LocoComp->MinDistanceToDistanceMatch = 1.f;
}

void UAimState::OnExitState()
{
	Super::OnExitState();
	LocoComp->MinDistanceToDistanceMatch = 30.f;

}

void UAimState::PushYawCorrection() const
{
	const UCameraModeDataAsset* CamDA = LocoComp->bIsCrouched ? AimCrouchCameraData : AimCameraData;
	if (!CamDA) return;
	
	const float LateralOffset = CamDA->SocketOffset.Y + CamDA->TargetSocket.Y;
	
	AnimInstance->AimYawCorrection = FMath::RadiansToDegrees(FMath::Atan2(LateralOffset, ConvergenceDistance));
}

void UAimState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
#pragma region Switches
	if (!LocoComp->bIsAiming)
	{
		if (LocoComp->bIsCrouched)
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
	
	if (FMath::Abs(LocoComp->RootYawOffset) > 0.1f)
	{
		LocoComp->RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			LocoComp->RootYawOffset, 0.f, SpringState,
			120.f, 1.f, DeltaTime);
	}
	else
	{
		LocoComp->RootYawOffset = 0.f;
	}
	
	
	if (LocoComp->bShouldMove || PlayerRef->GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
		UpdateOrientationDirection(DeltaTime);
	
	UpdateAnimationParameters(DeltaTime);
	
	
#pragma region DEBUG
#if ENABLE_DRAW_DEBUG
	if (const USkeletalMeshComponent* Mesh = PlayerRef->GetMesh())
	{
		UWorld* World = PlayerRef->GetWorld();
		FCollisionQueryParams QP(SCENE_QUERY_STAT(AimDebug), false, PlayerRef);
	
		// ROSSA: raggio canna, dal socket lungo il suo asse — include la correzione del busto
		const FTransform SockT = Mesh->GetSocketTransform(TEXT("WP_WolverineSocket"));
		const FVector BarrelDir = SockT.GetUnitAxis(EAxis::X);	// verifica asse/segno dal gizmo del socket
		FHitResult BarrelHit;
		if (World->LineTraceSingleByChannel(BarrelHit, SockT.GetLocation(), SockT.GetLocation() + BarrelDir * 10000.f, ECC_Visibility, QP))
			DrawDebugSphere(World, BarrelHit.ImpactPoint, 6.f, 12, FColor::Red, false, -1.f);
	
		// VERDE: raggio camera = punto sotto il crosshair
		FVector CamLoc; FRotator CamRot;
		if (APlayerController* PC = Cast<APlayerController>(PlayerRef->GetController()))
		{
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			FHitResult CamHit;
			if (World->LineTraceSingleByChannel(CamHit, CamLoc, CamLoc + CamRot.Vector() * 10000.f, ECC_Visibility, QP))
				DrawDebugSphere(World, CamHit.ImpactPoint, 6.f, 12, FColor::Green, false, -1.f);
		}
	}
#endif
	GEngine->AddOnScreenDebugMessage(80, 0.f, FColor::Orange,
    	FString::Printf(TEXT("YawCorr: %.2f | AimAlpha: %.2f | AimPitch: %.3f"),
    		AnimInstance->AimYawCorrection, AnimInstance->AimAlpha,
    		AnimInstance->AimPitch));
#pragma endregion
}

/* 
 *  Ricorda nei enter condition di mettere eventualmente i check se sta nei vari stop state in graph */