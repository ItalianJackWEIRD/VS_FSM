// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomComponents/LocomotionTypes.h"
#include "WeaponDataAsset.generated.h"

class AWeaponBase;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FStanceOverlaySet
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category="Overlay")
	TObjectPtr<UBlendSpace> ReadyStand = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Overlay")
	TObjectPtr<UBlendSpace> ReadyStand2H = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Overlay")
	TObjectPtr<UBlendSpace> ReadyCrouch = nullptr;
};

UCLASS(BlueprintType)
class VS_FSM_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<AWeaponBase> WeaponActorClass;
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName AttachSocketName = TEXT("WP_WolverineSocket");
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	EWeaponGrip Grip = EWeaponGrip::OneHand;
	
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Equip")
	TObjectPtr<UAnimMontage> EquipAnimation = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Overlay")
	FStanceOverlaySet OverlayAnims;
	
};
