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
	
	// -- AIM
	void SetAiming(bool bNewAiming);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimChangedSignature, bool, bAiming);
	UPROPERTY(BlueprintAssignable, Category="Shooting|Aim")
	FOnAimChangedSignature OnAimChanged;
	
	UPROPERTY(EditDefaultsOnly, Category="Shooting|Aim", meta=(ClampMin="10.0", ClampMax="90.0"))
	float MaxAimPitch = 55.f;	// oltre questo il busto non si piega più

protected:
	virtual void BeginPlay() override;
	
	// Pointers Helper
	UPROPERTY()
	TObjectPtr<UCustomAnimInstance> CustomAnimInstance = nullptr;
	UPROPERTY()
	TObjectPtr<UBreathingComponent> BreathingComponent = nullptr;
	
	struct FChannelTargets { float OneHand = 0.f; float TwoHand = 0.f; float Aim = 0.f; };
	FChannelTargets ComputeChannelTargets() const;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bIsShootingComponentActive = true;
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bHasWeapon;
	
	
	
private:
	void UpdateAimPose();
	bool bIsAiming = false;
	
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
	UPROPERTY(EditDefaultsOnly, Category="General Settings")
	float AimInterpSpeed = 8.f;	// tuning point, molto più alta della WeaponInterpSpeed
	
	bool bRunAlphaOverride = false;
	float BreathingAlpha = 0.f;
	
	bool bIsTransitioning = false;
	
	float ComputeTargetAlpha() const;
	EStanceMode GetStanceMode() const;
	
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HolsterMeshComp = nullptr;
	void SetupHolsterMesh();
	
	// Mixed : CQB
	UPROPERTY(EditDefaultsOnly, Category="CQB|Proximity")
	float CQBProbesRadius = 250.f;
	UPROPERTY(EditDefaultsOnly, Category="CQB|Proximity")
	float ProximityInterval = 0.08f; // ~ 12 al sec
	UPROPERTY(EditDefaultsOnly, Category="CQB|Proximity")
	TArray<TEnumAsByte<EObjectTypeQuery>> CoverObjectTypes;	// Channel "Cover"
	
	bool bInTightSpace = false;
	FTimerHandle ProximityTimerHandle;
	
	void StartProximityScan();
	void StopProximityScan();
	void TickProximityScan();
	bool ProbeForCover() const;
	

		
};