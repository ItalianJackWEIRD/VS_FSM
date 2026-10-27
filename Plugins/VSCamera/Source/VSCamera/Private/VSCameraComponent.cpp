// Fill out your copyright notice in the Description page of Project Settings.


#include "VSCameraComponent.h"
#include "CameraModeDataAsset.h"
#include "StateManagerComponent.h"
#include "VSCameraLeanModifier.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include  "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	// Se il modo precedente non aveva limite di lag (0), il nuovo limite parte da qui e si stringe piano.
	constexpr float UnlimitedLagStart = 500.f;
}

UVSCameraComponent::UVSCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}


void UVSCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheComponents();
	
	if (SpringArm) BlendedTargetOffset = SpringArm->TargetOffset;

	// La capsula scende nel tick del CMC: aggiorniamo dopo di lui, così la compensazione arriva nello stesso frame.
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
		AddTickPrerequisiteComponent(Char->GetCharacterMovement());
	
	if (AActor* Owner = GetOwner())
		if (UStateManagerComponent* SM = Owner->FindComponentByClass<UStateManagerComponent>())
			SM->OnStateChanged.AddUObject(this, &UVSCameraComponent::HandleStateChanged);
	
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
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetMode->SocketOffset,DeltaTime, Speed);
		BlendedTargetOffset     = FMath::VInterpTo(BlendedTargetOffset, TargetMode->TargetSocket, DeltaTime, Speed);
		SpringArm->TargetOffset = BlendedTargetOffset + FVector(0.f, 0.f, HeightCompensation);
		
		// Lag e sonda si interpolano come il resto: copiati al volo facevano saltare la camera al cambio di stato.
		SpringArm->CameraLagSpeed         = FMath::FInterpTo(SpringArm->CameraLagSpeed, TargetMode->CameraLagSpeed, DeltaTime, Speed);
		SpringArm->CameraRotationLagSpeed = FMath::FInterpTo(SpringArm->CameraRotationLagSpeed, TargetMode->CameraRotationLagSpeed, DeltaTime, Speed);
		SpringArm->ProbeSize              = FMath::FInterpTo(SpringArm->ProbeSize, TargetMode->ProbeSize, DeltaTime, Speed);

		// Max Distance: 0 vuol dire nessun limite. Toglierlo non fa salti, stringerlo sì: quindi si stringe piano.
		const float TargetLagMax = TargetMode->CameraLagMaxDistance;
		if (TargetLagMax <= 0.f)
		{
			SpringArm->CameraLagMaxDistance = 0.f;
		}
		else
		{
			if (SpringArm->CameraLagMaxDistance <= 0.f) SpringArm->CameraLagMaxDistance = UnlimitedLagStart;
			SpringArm->CameraLagMaxDistance = FMath::FInterpTo(SpringArm->CameraLagMaxDistance, TargetLagMax, DeltaTime, Speed);
		}
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

void UVSCameraComponent::HandleStateChanged(const FString& StateKey)
{
	if (const TObjectPtr<UCameraModeDataAsset>* Found = StateModes.Find(StateKey))
		SetCameraMode(*Found);
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
		SpringArm->bDoCollisionTest         = Mode->bDoCollisionTest;
	}
	
	if (Immediate)
	{
		if (SpringArm)
		{
			SpringArm->TargetArmLength          = Mode->TargetArmLength;
			SpringArm->SocketOffset             = Mode->SocketOffset;
			BlendedTargetOffset     = Mode->TargetSocket;
			SpringArm->TargetOffset = BlendedTargetOffset + FVector(0.f, 0.f, HeightCompensation);
			SpringArm->CameraLagSpeed           = Mode->CameraLagSpeed;
			SpringArm->CameraRotationLagSpeed   = Mode->CameraRotationLagSpeed;
			SpringArm->CameraLagMaxDistance     = Mode->CameraLagMaxDistance;
			SpringArm->ProbeSize                = Mode->ProbeSize;
		}
		
		if (Camera)
			Camera->SetFieldOfView(Mode->FieldOfView);
	}
		
	
}

