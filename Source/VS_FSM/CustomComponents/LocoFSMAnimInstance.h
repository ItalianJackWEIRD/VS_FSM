// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
#include "Animation/AnimInstance.h"
#include "LocoFSMAnimInstance.generated.h"

class ULocomotionStateComponent;
class UCharacterMovementComponent;

/**
 * Parent class di ABP_Loco_FSM, il layer di locomotion.
 *
 * Deriva da UAnimInstance puro: non conosce AVS_FSMCharacter né UCustomAnimInstance.
 * Il layer GASP starà allo stesso livello, non sotto questa classe.
 *
 * Fa una cosa sola: copia il componente in membri propri sul game thread, una volta
 * per frame, così il grafo legge dati locali invece di dereferenziare un UObject
 * esterno da un worker thread. Stessi nomi di prima, quindi il reparent
 * dell'ABP duplicato riaggancia i nodi per nome.
 *
 * NON contiene il blocco feedback (bAnimGraphIn*, bShouldPivot, bShouldRecenterIdle,
 * bIsIdleBreak): quelle le SCRIVE il grafo, quindi una copia locale le perderebbe.
 * Vanno scritte direttamente sul componente, via il membro LocoComp.
 */
UCLASS()
class VS_FSM_API ULocoFSMAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Il componente, per i nodi che devono SCRIVERE (blocco feedback). */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UCharacterMovementComponent* CharacterMovement = nullptr;

#pragma region LOCOMOTION

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector VelocityXY = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouched = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EMovementGait MovementGait = EMovementGait::Walk;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float OrientationAngle = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector SmoothedDir = FVector::ForwardVector;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Fwd = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Bwd = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Left = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Right = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LeanAngle = 0.f;
	UPROPERTY(BlueprintReadOnly)
	int LeanStateIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLeftShoulderLocomotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion-Pivot")
	TObjectPtr<UAnimSequence> PivotAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion-Pivot")
	float PivotStartTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion-Pivot")
	float PivotEndTimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bMovStopCrouched = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Jog")
	bool bMovStopJogging = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Run")
	bool bTransitionRunInJog = false;

	UPROPERTY(BlueprintReadOnly, Category = "General ABP Settings")
	float PlayRate = 1.f;

#pragma endregion

#pragma region TURNING

	UPROPERTY(BlueprintReadOnly)
	float RootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float LastRootYawOffset = 0.f;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnRight = false;
	UPROPERTY(BlueprintReadOnly)
	bool bShouldTurnLeft = false;
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float TurnThreshold = 50.f;
	UPROPERTY(BlueprintReadOnly)
	float TurnAnimElapsedTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalTurnAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	FString RemainingTurnYawCurveName = FString(TEXT("RemainingTurnYaw"));

#pragma endregion

#pragma region CLIPS

	UPROPERTY(BlueprintReadOnly, Category="Idle")
	UAnimSequence* FinalIdleAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	UAnimSequence* FinalIdleBreakAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	UAnimSequence* FinalIdleRecenterAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	UAnimSequence* FinalStanceTransitionAnim = nullptr;

#pragma endregion

#pragma region DISTANCE MATCHING

	UPROPERTY(BlueprintReadOnly)
	bool bUseSeparateBrakingFriction = false;
	UPROPERTY(BlueprintReadOnly)
	float BrakingFriction = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float GroundFriction = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float BrakingFrictionFactor = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float BrakingDecelerationWalking = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float MinDistanceToDistanceMatch = 30.f;

#pragma endregion

	/** Copia locale: niente puntatori esterni dereferenziati da un worker thread. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	EStanceMode GetStanceMode() const { return StanceMode; }

	// --- Trigger one-shot, inoltrati al componente.
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldIdleBreak();
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldMovWalkJogStanceTransition();

	// --- Le clip di transizione le suona questo layer, quindi le notify arrivano qui.
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EStanceMode StanceMode = EStanceMode::Normal;

private:
	bool EnsureLocoComp();
	void PullFromComponent();

	/** Warning una volta sola: EnsureLocoComp gira ogni frame. */
	bool bWarnedMissingComp = false;
};