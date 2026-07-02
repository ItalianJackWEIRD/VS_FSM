// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/ShootingSystem.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "ShootingSystem//WeaponDataAsset.h"
#include "ShootingSystem//WeaponBase.h"
#include "VS_FSMCharacter.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "CustomComponents/CustomAnimInstance.h" 

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<bool> CVarShowCQBDebug(
	TEXT("MyGame.ShowCQBDebug"),
	false,   // default: disattivato
	TEXT("Mostra le sfere di debug per i CQB probes (0 = off, 1 = on)"),
	ECVF_Default
);
#endif

// Sets default values for this component's properties
UShootingSystem::UShootingSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShootingSystem::SelectWeapon(UWeaponDataAsset* NewWeapon)
{
	CurrentWeaponData = NewWeapon;
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(true);
}

void UShootingSystem::Arm()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!CurrentWeaponData || !CurrentWeaponData->WeaponActorClass || !Mesh) return;
	
	if (EquippedWeapon) Disarm();
	
	BreathingComponent->SwitchOn();
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(false);
	
	const FName Socket = CurrentWeaponData->AttachSocketName;
	const FTransform SocketTransform = Mesh->GetSocketTransform(Socket);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	EquippedWeapon = GetWorld()->SpawnActor<AWeaponBase>(CurrentWeaponData->WeaponActorClass, SocketTransform, SpawnParams);
	
	if (!EquippedWeapon) return;
	
	bHasWeapon = true;
	
	EquippedWeapon->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	
	if (CustomAnimInstance)		// Push anim on ABP
	{
		CustomAnimInstance->WeaponGrip = CurrentWeaponData->Grip;
		CustomAnimInstance->Overlay1HStand = CurrentWeaponData->Overlay1H.ReadyStand;
		CustomAnimInstance->Overlay1HCrouch = CurrentWeaponData->Overlay1H.ReadyCrouch;
		CustomAnimInstance->Overlay2HStand     = CurrentWeaponData->Overlay2H.ReadyStand;
		CustomAnimInstance->Overlay2HCrouch    = CurrentWeaponData->Overlay2H.ReadyCrouch;
	}
	if (CurrentWeaponData->Grip == EWeaponGrip::Mixed)
		StartProximityScan();
	
	bIsTransitioning = false;
	UpdateAimPose();
}

void UShootingSystem::Disarm()
{
	bHasWeapon 	= false;
	if (EquippedWeapon) EquippedWeapon->Destroy();
	EquippedWeapon 	= nullptr;
	bIsTransitioning = false;
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(true);
	CustomAnimInstance->bUpperBodyOn = false;
	BreathingComponent->SwitchOff();
	StopProximityScan(); // Always, Fallback...
}


void UShootingSystem::SetWeaponEquip()
{
	// WHOLE LOGIC: Set Abp parameters (Which is put false in OnEventExitBP) , On/Off Breathing System in Arm() Disarm(), set Alpha to 1 (bIsTransitioning).
	if (!CurrentWeaponData) return;
	
	CustomAnimInstance->bUpperBodyOn = true;
	
	bIsTransitioning = true;
	CustomAnimInstance->bShouldEquipWeapon = true;
	
	if (!bHasWeapon)
	{
		if (CustomAnimInstance->bIsCrouched)
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->EquipAnimationCrouch;
		else
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->EquipAnimationStand;
	}
	else
	{
		if (CustomAnimInstance->bIsCrouched)
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->UnEquipAnimationCrouch;
		else
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->UnEquipAnimationStand;
	}
}

void UShootingSystem::SetAiming(bool bNewAiming)
{
	if (bIsAiming == bNewAiming) return;
	if (bNewAiming && (!bHasWeapon || bIsTransitioning)) return;
	
	bIsAiming = bNewAiming;
	if (CustomAnimInstance) CustomAnimInstance->bIsAiming = bNewAiming;
}

void UShootingSystem::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		CustomAnimInstance = Cast<UCustomAnimInstance>(Mesh->GetAnimInstance());
		BreathingComponent = Cast<UBreathingComponent>(GetOwner()->FindComponentByClass<UBreathingComponent>());
	}

	if (!CustomAnimInstance)
		UE_LOG(LogTemp, Warning, TEXT("ShootingSystem: CustomAnimInstance nulla a BeginPlay"));
	
	// Default di test finché non arriva l'Inventory col D-pad. Mettiamo tutto in SetupNewWeapon()
	if (!CurrentWeaponData)
		CurrentWeaponData = DefaultWeaponData;
	
	SetupHolsterMesh();
}


UShootingSystem::FChannelTargets UShootingSystem::ComputeChannelTargets() const
{
	FChannelTargets T;
	if (!bHasWeapon && !bIsTransitioning) return T;
	if (!CurrentWeaponData) return T;
	
	const float Base = bIsTransitioning ? 1.f : ComputeTargetAlpha();

	if (bIsAiming)
	{
		T.TwoHand = 1.f;   // try
		T.Aim     = 1.f;
		return T;
	}

	switch (CurrentWeaponData->Grip)
	{
	case EWeaponGrip::OneHand:  T.OneHand = Base; break;
	case EWeaponGrip::TwoHand:  T.TwoHand = Base; break;
	case EWeaponGrip::Mixed:
		if (bInTightSpace) T.TwoHand = Base;
		else               T.OneHand = Base;
		break;
	default: break;
	}
	return T;
}

void UShootingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CustomAnimInstance || !CurrentWeaponData) return;
	
	UpdateAimPose();
	
	const FChannelTargets T = ComputeChannelTargets();
	
	const float InterpSpeed = bIsAiming ? AimInterpSpeed : WeaponInterpSpeed;

	CustomAnimInstance->Weapon1hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon1hAlpha, T.OneHand, DeltaTime, InterpSpeed);
	CustomAnimInstance->Weapon2hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon2hAlpha, T.TwoHand, DeltaTime, InterpSpeed);
	CustomAnimInstance->AimAlpha      = FMath::FInterpTo(CustomAnimInstance->AimAlpha,      T.Aim,     DeltaTime, InterpSpeed);

	// gate dei nodi LBP: "acceso" = ha peso residuo, così il blend-out finisce sempre
	CustomAnimInstance->bUpper1H = CustomAnimInstance->Weapon1hAlpha > 0.01f;
	CustomAnimInstance->bUpper2H = CustomAnimInstance->Weapon2hAlpha > 0.01f;

	const float MaxAlpha = FMath::Max(CustomAnimInstance->Weapon1hAlpha, CustomAnimInstance->Weapon2hAlpha);
	CustomAnimInstance->GripAlpha = bHasWeapon ? (1.f - MaxAlpha) : 0.f;
	
	GEngine->AddOnScreenDebugMessage(77, 0.f, FColor::Yellow,
	FString::Printf(TEXT("Upper: %d 1hA: %f 2hA: %f TightSpace: %s"), CustomAnimInstance->bUpperBodyOn, CustomAnimInstance->Weapon1hAlpha, CustomAnimInstance->Weapon2hAlpha, bInTightSpace ? TEXT("TRUE") : TEXT("FALSE")));
	GEngine->AddOnScreenDebugMessage(78, 0.f, FColor::Green,
	FString::Printf(TEXT("bIsCrouched: %d FinalAimAnim: %s"),
		CustomAnimInstance->bIsCrouched ? 1 : 0,
		*GetNameSafe(CustomAnimInstance->FinalAimPose)));
}

void UShootingSystem::UpdateAimPose()
{
	if (!CustomAnimInstance || !CurrentWeaponData) return;
	
	UAnimSequence* Desired = CustomAnimInstance->bIsCrouched
		? CurrentWeaponData->AimPoseCrouch
		: CurrentWeaponData->AimPoseStand;
	
	if (CustomAnimInstance->FinalAimPose != Desired)
		CustomAnimInstance->FinalAimPose = Desired;
}

USkeletalMeshComponent* UShootingSystem::GetOwnerMesh() const
{
	if (const ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		return Owner->GetMesh();
	}
	return nullptr;
}

float UShootingSystem::ComputeTargetAlpha() const
{
	if (!bHasWeapon) return 0.f;
	if (bRunAlphaOverride) return 0.f;
	
	const float StanceBase = (GetStanceMode() == EStanceMode::Alert) ? 1.f : 0.f;
	return FMath::Max(StanceBase, BreathingAlpha);		// if u are in normal, breathing takes over, otherwise in alert its always 1.
}

EStanceMode UShootingSystem::GetStanceMode() const
{
	if (const AVS_FSMCharacter* Char = Cast<AVS_FSMCharacter>(GetOwner()))
		return Char->GetStanceMode();
	return EStanceMode::Normal; //Fallback
}

void UShootingSystem::SetupHolsterMesh()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh || !CurrentWeaponData || !CurrentWeaponData->HolsterMesh) return;

	const FName Socket = CurrentWeaponData->HolsterSocketName;
	if (!Mesh->DoesSocketExist(Socket))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShootingSystem: holster socket '%s' inesistente sullo skeleton in-game"), *Socket.ToString());
		return;
	}

	if (!HolsterMeshComp)
	{
		HolsterMeshComp = NewObject<UStaticMeshComponent>(GetOwner());
		HolsterMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HolsterMeshComp->RegisterComponent();
	}

	HolsterMeshComp->SetStaticMesh(CurrentWeaponData->HolsterMesh);
	HolsterMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	HolsterMeshComp->SetVisibility(!bHasWeapon); // visibile solo quando l'arma NON è in mano
}


/* MIXED SYSTEM */

void UShootingSystem::StartProximityScan()
{
	UWorld* World = GetWorld();
	if (!World) return;
	World->GetTimerManager().SetTimer(ProximityTimerHandle, this, &UShootingSystem::TickProximityScan, ProximityInterval, true);
	TickProximityScan();
}

void UShootingSystem::StopProximityScan()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(ProximityTimerHandle);
	
	bInTightSpace = false;
}

void UShootingSystem::TickProximityScan()
{
	const bool bNow = ProbeForCover();
	if (bNow == bInTightSpace) return;
	
	bInTightSpace = bNow;
}

bool UShootingSystem::ProbeForCover() const
{
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World || CoverObjectTypes.Num() == 0) return false;
	
	FCollisionObjectQueryParams ObjParams(CoverObjectTypes);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CQBProximity), false);
	QueryParams.AddIgnoredActor(Owner);
	
	const bool bHit = World->OverlapAnyTestByObjectType(
		Owner->GetActorLocation(),
		FQuat::Identity,
		ObjParams,
		FCollisionShape::MakeSphere(CQBProbesRadius),
		QueryParams);
	
#if ENABLE_DRAW_DEBUG
	if (CVarShowCQBDebug.GetValueOnGameThread())
	{
		DrawDebugSphere(World, Owner->GetActorLocation(), CQBProbesRadius, 16,
			bHit ? FColor::Red : FColor::Green, false, ProximityInterval);
	}
#endif

	return bHit;
}
