// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/AnimNotify_ShootingEquip.h"
#include "CustomComponents/ShootingSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotify_ShootingEquip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	if (UShootingSystem* Shooting = Owner->FindComponentByClass<UShootingSystem>())
	{
		if (bArm) Shooting->Arm();
		else      Shooting->Disarm();
	}
}

FString UAnimNotify_ShootingEquip::GetNotifyName_Implementation() const
{
	return bArm ? TEXT("Shooting Arm") : TEXT("Shooting Disarm");
}
