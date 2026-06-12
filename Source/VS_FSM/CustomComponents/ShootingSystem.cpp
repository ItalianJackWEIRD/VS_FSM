// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/ShootingSystem.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

#include "CustomComponents/CustomAnimInstance.h" 

// Sets default values for this component's properties
UShootingSystem::UShootingSystem()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UShootingSystem::Arm()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!EquippedWeaponClass || !Mesh) return;
	
	bHasWeapon = true;
	
	const FTransform SocketTransform = Mesh->GetSocketTransform(AttachSocketName);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	EquippedWeapon = GetWorld()->SpawnActor<AActor>(EquippedWeaponClass, SocketTransform, SpawnParams);
	
	if (!EquippedWeapon) return;
	
	if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(EquippedWeapon->GetRootComponent()))
	{
		Root->SetSimulatePhysics(false);
		Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	EquippedWeapon->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
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
}


// Called every frame
void UShootingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Target = bHasWeapon ? 1.f : 0.f;
	CustomAnimInstance->WeaponAlpha = FMath::FInterpTo(CustomAnimInstance->WeaponAlpha, Target, DeltaTime, WeaponInterpSpeed);
}

USkeletalMeshComponent* UShootingSystem::GetOwnerMesh() const
{
	if (const ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		return Owner->GetMesh();
	}
	return nullptr;
}

