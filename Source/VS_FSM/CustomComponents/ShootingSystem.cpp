// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/ShootingSystem.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "ShootingSystem//WeaponDataAsset.h"
#include "ShootingSystem//WeaponBase.h"
#include "VS_FSMCharacter.h"
#include "CustomComponents/CustomAnimInstance.h" 


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
	
	if (CustomAnimInstance)
	{
		CustomAnimInstance->WeaponGrip = CurrentWeaponData->Grip;
		CustomAnimInstance->OverlayReadyStand = CurrentWeaponData->OverlayAnims.ReadyStand;
		CustomAnimInstance->OverlayReadyCrouch = CurrentWeaponData->OverlayAnims.ReadyCrouch;
	}
	bIsTransitioning = false;
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
	
	// Default di test finché non arriva l'Inventory col D-pad.
	if (!CurrentWeaponData)
		CurrentWeaponData = DefaultWeaponData;
	
	SetupHolsterMesh();
}


void UShootingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CustomAnimInstance) return;
	
	if (bIsTransitioning)
	{
		CustomAnimInstance->WeaponAlpha = FMath::FInterpTo(CustomAnimInstance->WeaponAlpha, 1, DeltaTime, WeaponInterpSpeed);;
		return;
	}
	
	const float Target = ComputeTargetAlpha();
	CustomAnimInstance->WeaponAlpha = FMath::FInterpTo(CustomAnimInstance->WeaponAlpha, Target, DeltaTime, WeaponInterpSpeed);
	CustomAnimInstance->GripAlpha = bHasWeapon ? (1.f - CustomAnimInstance->WeaponAlpha) : 0.f; // cause weaponAlpha is already interpolated
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