// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LocomotionTypes.h"
#include "ShootingSystem/BreathingComponent.h"
#include "ShootingSystem.generated.h"

class UCustomAnimInstance;
class UWeaponDataAsset;
class AWeaponBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VS_FSM_API UShootingSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	UShootingSystem();
	void SelectWeapon(UWeaponDataAsset* NewWeapon); // doesnt spawn, just changes its
	void Arm();	// equip the selected Weapon
	void Disarm();
	
	void SetRunStateAlphaOverride(bool bActive) { bRunAlphaOverride = bActive; }
	void SetBreathingAlpha(float inAlpha) { BreathingAlpha = FMath::Clamp(inAlpha, 0.f, 1.f); }
	
	void SetWeaponEquip();

protected:
	virtual void BeginPlay() override;
	
	// Pointers Helper
	UPROPERTY()
	TObjectPtr<UCustomAnimInstance> CustomAnimInstance = nullptr;
	UPROPERTY()
	TObjectPtr<UBreathingComponent> BreathingComponent = nullptr;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bIsShootingComponentActive = true;
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bHasWeapon;
	
private:
	UPROPERTY()		// what is SELECTED
	TObjectPtr<UWeaponDataAsset> CurrentWeaponData = nullptr;
	
	UPROPERTY()		// what is EQUIPPED (equipped means currently using, different than "selected")
	TObjectPtr<AWeaponBase> EquippedWeapon = nullptr;
	
	// Default di test finché non c'è l'Inventory.
	UPROPERTY(EditDefaultsOnly, Category="Weapon Component")
	TObjectPtr<UWeaponDataAsset> DefaultWeaponData = nullptr;
	
	USkeletalMeshComponent* GetOwnerMesh() const;
	
	UPROPERTY(EditDefaultsOnly, Category= "General Settings")
	float WeaponInterpSpeed = 8.f;
	
	bool bRunAlphaOverride = false;
	float BreathingAlpha = 0.f;
	
	bool bIsTransitioning = false;
	
	float ComputeTargetAlpha() const;
	EStanceMode GetStanceMode() const;
	
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HolsterMeshComp = nullptr;
	void SetupHolsterMesh();
	
	void SetupNewWeapon();	// set in ABP the bool for the type of weapon

		
};