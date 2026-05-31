// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EquipComponent.generated.h"

class USkeletalMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VS_FSM_API UEquipComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UEquipComponent();
	
	void Equip();
	void UnEquip();

protected:
	// Quale item spawnare. TSubclassOf<AActor> per ora; quando farai AEquippableItem, restringi a quello
	UPROPERTY(EditDefaultsOnly, Category= "Equip Component")
	TSubclassOf<AActor> EquippedActorClass;
	
	UPROPERTY(EditDefaultsOnly, Category= "Equip Component")
	FName AttachSocketName = TEXT("flare_socket_r");

private:
	bool bEquipped = false;
	
	UPROPERTY()
	TObjectPtr<AActor> EquippedActor = nullptr;
	
	USkeletalMeshComponent* GetOwnerMesh() const;
	
	UPROPERTY(EditDefaultsOnly, Category= "Flare Options")
	float FlareVelocity = 150.f;
	UPROPERTY(EditDefaultsOnly, Category= "Flare Options")
	float FlareDestroyTime = 10.f;

};
