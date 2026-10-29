// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IkContextComponent.generated.h"

class ACharacter;
class UCapsuleComponent;
class UCharacterMovementComponent;
class ULocomotionStateComponent;

/**
 * IK contestuali: un componente solo per tutte le meccaniche (shoulder avoidance, poi le altre).
 *
 * Ogni meccanica ha impostazioni, stato e funzione di update sue.
 * In comune ci sono solo il tick, i riferimenti e la scrittura nel LocoComp.
 *
 * Interruttore e permesso li legge dal LocoComp. Valore e alpha li scrive nel LocoComp,
 * e ne è l'unico scrittore. Unico scrittore anche del raggio della capsula.
 *
 * Tick: dopo il CMC e prima della mesh, come il LocoComp.
 */
UCLASS(ClassGroup=(Locomotion), meta=(BlueprintSpawnableComponent))
class ULS_API UIkContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIkContextComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

// ============================================================================
// SHOULDER AVOIDANCE
// ============================================================================
#pragma region SHOULDER_AVOIDANCE

	// --- Impostazioni: UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	//     sweep: raggio, spostamento laterale, lunghezza
	//     distanze: valore a zero, valore pieno, isteresi
	//     raggio ristretto della capsula

	// --- Stato: membri semplici, niente UPROPERTY
	//     valore corrente, colpo a sinistra / a destra nel frame prima, raggio della capsula
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderRiseSpeed = 4.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderFallSpeed = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(tooltip="Fast Fall, in case the Mechanic is off."))
	float ShoulderExitSpeed = 4.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderSnapThreshold = 0.01f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderMinSpeed = 10.f;
	
	float TargetShoulderWeight = 0.f;

	void UpdateShoulderAvoidance(float DeltaTime);

#pragma endregion

private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Capsule = nullptr;
};