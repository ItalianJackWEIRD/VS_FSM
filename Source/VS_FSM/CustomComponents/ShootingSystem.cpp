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
}

void UShootingSystem::Arm()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!CurrentWeaponData || !CurrentWeaponData->WeaponActorClass || !Mesh) return;
	
	if (EquippedWeapon) Disarm();
	
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
		
}

void UShootingSystem::Disarm()
{
	bHasWeapon 	= false;
	if (EquippedWeapon) EquippedWeapon->Destroy();
	EquippedWeapon 	= nullptr;
	
}


void UShootingSystem::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
		CustomAnimInstance = Cast<UCustomAnimInstance>(Mesh->GetAnimInstance());

	if (!CustomAnimInstance)
		UE_LOG(LogTemp, Warning, TEXT("ShootingSystem: CustomAnimInstance nulla a BeginPlay"));
	
	// Default di test finché non arriva l'Inventory col D-pad.
	if (!CurrentWeaponData)
		CurrentWeaponData = DefaultWeaponData;
}


void UShootingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CustomAnimInstance) return;
	
	CustomAnimInstance->bWeaponEquipped = bHasWeapon;
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

