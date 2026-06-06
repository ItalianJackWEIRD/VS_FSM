// Fill out your copyright notice in the Description page of Project Settings.


#include "VSCameraComponent.h"
#include "CameraModeDataAsset.h"
#include "VSCameraLeanModifier.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include  "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"

UVSCameraComponent::UVSCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}


void UVSCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheComponents();
	
	if (DefaultMode)
		SetCameraMode(DefaultMode, false);
	else
		UE_LOG(LogTemp, Warning, TEXT("[VSCamera] DefaultMode non assegnata su %s"), *GetNameSafe(GetOwner()));
}

void UVSCameraComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!TargetMode) return;
	
	const float Speed = TargetMode->BlendInterpSpeed;
	
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetMode->TargetArmLength,DeltaTime, Speed);
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetMode->SocketOffset,DeltaTime, Speed);;
		SpringArm->TargetOffset = FMath::VInterpTo(SpringArm->TargetOffset, TargetMode->TargetSocket,DeltaTime, Speed);;
	}
	if (Camera)
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetMode->FieldOfView,DeltaTime, Speed));
	
#pragma region LEAN
	
	RegisterLeanModifiers();
	
	const float Target = FMath::Clamp(LeanInput * TargetMode->LeanMultiplier, -TargetMode->MaxLeanAngle, TargetMode->MaxLeanAngle);
	CurrentLeanRoll = FMath::FInterpTo(CurrentLeanRoll, Target, DeltaTime, TargetMode->LeanInterpSpeed);
	
#pragma endregion
	
}


void UVSCameraComponent::CacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;
	
	SpringArm = Owner->FindComponentByClass<USpringArmComponent>();
	Camera = Owner->FindComponentByClass<UCameraComponent>();
	
	if (!SpringArm) UE_LOG(LogTemp, Warning, TEXT("[VSCamera] Nessuno SpringArm su %s"), *GetNameSafe(Owner));
	if (!Camera)    UE_LOG(LogTemp, Warning, TEXT("[VSCamera] Nessuna CameraComponent su %s"), *GetNameSafe(Owner));
	
}

void UVSCameraComponent::RegisterLeanModifiers()
{
	if (bModifiersRegistered) return;
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC || !PC->PlayerCameraManager) return; // pawn non ancora posseduto -> riprova il prossimo tick
	
	LeanModifier = Cast<UVSCameraLeanModifier>(PC->PlayerCameraManager->AddNewCameraModifier(UVSCameraLeanModifier::StaticClass()));
	
	if (LeanModifier)
	{
		LeanModifier->OwnerComp = this;
		bModifiersRegistered = true;
	}
}


void UVSCameraComponent::SetCameraMode(UCameraModeDataAsset* Mode, bool Immediate)
{
	if (!Mode) return;
	TargetMode = Mode;
	
	if (SpringArm)
	{
		SpringArm->bEnableCameraLag         = Mode->bEnableCameraLag;
		SpringArm->bEnableCameraRotationLag = Mode->bEnableCameraRotationLag;
		SpringArm->CameraLagSpeed           = Mode->CameraLagSpeed;
		SpringArm->CameraRotationLagSpeed   = Mode->CameraRotationLagSpeed;
		SpringArm->CameraLagMaxDistance     = Mode->CameraLagMaxDistance;
		SpringArm->bDoCollisionTest         = Mode->bDoCollisionTest;
		SpringArm->ProbeSize                = Mode->ProbeSize;
	}
	
	if (Immediate)
	{
		if (SpringArm)
		{
			SpringArm->TargetArmLength          = Mode->TargetArmLength;
			SpringArm->SocketOffset             = Mode->SocketOffset;
			SpringArm->TargetOffset             = Mode->TargetSocket;
		}
		
		if (Camera)
			Camera->SetFieldOfView(Mode->FieldOfView);
	}
		
	
}

