// Fill out your copyright notice in the Description page of Project Settings.


#include "Equips/EquipComponent.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UEquipComponent::UEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipComponent::Equip()
{
	if (bEquipped) return;
	
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!EquippedActorClass || !Mesh) return;
	
	bEquipped = true;
	
	const FTransform SocketTransform = Mesh->GetSocketTransform(AttachSocketName);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	EquippedActor = GetWorld()->SpawnActor<AActor>(EquippedActorClass,SocketTransform,SpawnParams);
	if (!EquippedActor) return;
	
	if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(EquippedActor->GetRootComponent()))
	{
		Root->SetSimulatePhysics(false);
		Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	EquippedActor->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
}

void UEquipComponent::UnEquip()
{
	if (!bEquipped) return;
	
	bEquipped = false;
	
	if (EquippedActor)
	{
		EquippedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(EquippedActor->GetRootComponent()))
		{
			Root->SetCollisionProfileName(TEXT("DroppedItem"));
			Root->SetSimulatePhysics(true);
			
			const FVector ThrowDir = (-FVector::UpVector + GetOwner()->GetActorForwardVector() * 0.4f).GetSafeNormal(); //initial burst of speed
			Root->SetPhysicsLinearVelocity(ThrowDir * FlareVelocity);
		}
		EquippedActor->SetLifeSpan(FlareDestroyTime); // cambialo a seconda del tipo di BP
		EquippedActor = nullptr;
	}
}

USkeletalMeshComponent* UEquipComponent::GetOwnerMesh() const
{
	if (const ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		return Owner->GetMesh();
	}
	return nullptr;
}
