// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomComponents/LocomotionTypes.h"
#include "WeaponDataAsset.generated.h"

class AWeaponBase;
class UAnimMontage;

// Forward-looking: qui ci finiranno le anim di overlay (Stand/Crouch x stance).
// Lo riempiamo in step 3, per ora è solo il contenitore.
USTRUCT(BlueprintType)
struct FStanceOverlaySet
{
	GENERATED_BODY()
	// TO DO STEP 3
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
