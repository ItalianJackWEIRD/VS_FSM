// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShootingSystem.generated.h"

class UCustomAnimInstance;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VS_FSM_API UShootingSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	UShootingSystem();
	
	void Arm();
	void Disarm();

protected:
	virtual void BeginPlay() override;
	
	// Pointers Helper
	UPROPERTY()
	TObjectPtr<UCustomAnimInstance> CustomAnimInstance = nullptr;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bIsShootingActive = true;
	UPROPERTY(EditDefaultsOnly, Category= "Debug")
	bool bHasWeapon;
	
private:
	UPROPERTY()
	TObjectPtr<AActor> EquippedWeapon = nullptr;
	
	USkeletalMeshComponent* GetOwnerMesh() const;
	
	UPROPERTY(EditDefaultsOnly, Category= "General Settings")
	float WeaponInterpSpeed = 8.f;
	
	UPROPERTY(EditDefaultsOnly, Category= "Weapon Component")
	TSubclassOf<AActor> EquippedWeaponClass;
	
	UPROPERTY(EditDefaultsOnly, Category= "Weapon Component")
	FName AttachSocketName = TEXT("WP_WolverineSocket");

		
};
