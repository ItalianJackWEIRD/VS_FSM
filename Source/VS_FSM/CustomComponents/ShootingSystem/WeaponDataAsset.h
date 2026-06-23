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

UENUM(BlueprintType)
enum class EWeaponGrip : uint8		// add 2H Rifle
{
	OneHand UMETA(DisplayName="One Hand"),   
	TwoHand UMETA(DisplayName="Two Hand"), 
	Mixed UMETA(DisplayName="Mixed"),
	Melee   UMETA(DisplayName="Melee")       
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
	TObjectPtr<UAnimSequence> EquipAnimationStand = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Equip")
	TObjectPtr<UAnimSequence> EquipAnimationCrouch = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Equip")
	TObjectPtr<UAnimSequence> UnEquipAnimationStand = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Equip")
	TObjectPtr<UAnimSequence> UnEquipAnimationCrouch = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Holster")
	TObjectPtr<UStaticMesh> HolsterMesh = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Holster")
	FName HolsterSocketName = TEXT("HolsterSocket");
	
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Overlay")
	FStanceOverlaySet OverlayAnims;
	
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Overlay")
	UAnimSequence* Anim2H;
	
};