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
	// Probe and Capsule
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="Capsule Radius during Avoidance"))
	float ShoulderCapsuleRadius = 28.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderSweepRadius = 25.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="We set the height of the Sweep from this bone."))
	FName ShoulderBoneHeight = TEXT("spine_05");
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderLateralOffset = 20.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="After this distance Target = 0."))
	float ShoulderZeroDistance = 120.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="Below/Equal this distance Target is always = 1."))
	float ShoulderFullDistance = 40.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="If a Sweep is Succesful, we keep that hit for + this distance."))
	float ShoulderHysterisis = 15.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(ToolTip="We check in front of us of: SideDistance + SweepRadius + This Tolerance."))
	float ShoulderWallTolerance = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderRiseSpeed = 4.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance")
	float ShoulderFallSpeed = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(tooltip="Fast Fall, in case the Mechanic is off."))
	float ShoulderExitSpeed = 4.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(tooltip="Below this speed we enter Stationary Mode."))
	float ShoulderStationarySpeed = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category="ContextualIK|ShoulderAvoidance", meta=(tooltip="Below this it gets reset to 0."))
	float ShoulderSnapThreshold = 0.01f;
	
	/** --> State */
	float TargetShoulderWeight = 0.f;
	bool bShoulderHitLeftPrev = false;
	bool bShoulderHitRightPrev = false;
	int32 ShoulderBoneIndex = INDEX_NONE;
	
	float LastShoulderDistance = -1.f;		// solo debug: -1 = nessun colpo
	bool bLastShoulderStationary = false;	// solo debug

	void UpdateShoulderAvoidance(float DeltaTime);
	
	float ProbeShoulderTarget();
	bool SweepShoulder(const FVector& Start, const FVector& End, float Radius, float& OutDistance) const;
	void UpdateShoulderCapsule(bool bWantsNarrow);
	void DrawShoulderSweep(const FVector& Start, const FVector& End, float Radius, bool bHit, const FVector& HitPoint) const;
	void DebugShoulderAvoidance() const;

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
	
	// Saved on BeginPlay
	float DefaultCapsuleRadius = 0.f;
};