// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/LocomotionTypes.h"
#include "Animation/AnimInstance.h"
#include "LocoMMAnimInstance.generated.h"

class ULocomotionStateComponent;

/**
 * Parent class di ABP_Loco_MM, il layer di locomotion Motion Matching.
 *
 * Sorella di ULocoFSMAnimInstance, non figlia: deriva da UAnimInstance puro e non
 * conosce né la FSM né il character.
 *
 * Stessa regola della FSM: il grafo non tocca MAI il componente. Il C++ copia il
 * contratto nei membri in NativeUpdateAnimation, sul game thread; chooser e nodo MM
 * leggono solo le copie, anche dal worker thread.
 *
 * Differenze rispetto alla FSM:
 *   - niente SyncFeedback: il grafo MM non scrive niente sul componente.
 *     I bAnimGraphIn* sono feedback privato della FSM e qui non esistono.
 *   - solo il contratto: nessuna variabile dei blocchi FSM.
 *   - niente puntatore al CMC esposto: tutto ciò che serve arriva copiato.
 */
UCLASS()
class ULS_API ULocoMMAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

#pragma region CONTRACT

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	FVector VelocityXY = FVector::ZeroVector;

	/** Accelerazione live dal CMC, copiata ogni frame. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bIsAiming = false;

	/** Colonna della chooser dalla milestone 2. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	EMovementGait MovementGait = EMovementGait::Walk;

	/** Colonna della chooser: Alert / Normal. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	EStanceMode StanceMode = EStanceMode::Normal;

#pragma endregion

#pragma region ORIENTATION_LEAN

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Orientation")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;

	/**
	 * Direzione di movimento rispetto a dove guarda il personaggio, in gradi.
	 * È LocoComp->Fwd: al MM serve un angolo solo, non le quattro varianti cardinali.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Orientation")
	float LocomotionAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Lean")
	float LeanAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Lean")
	int32 LeanStateIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|PlayRate")
	float PlayRate = 1.f;

#pragma endregion

	// TRAIETTORIA: arriva al passo 3. La genera il componente, qui se ne copia il risultato.

private:
	/** NON esposto al Blueprint, come nella FSM: il grafo lavora solo sulle copie. */
	UPROPERTY()
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	bool EnsureLocoComp();
	void PullFromComponent();

	/** Warning una volta sola: EnsureLocoComp gira ogni frame. */
	bool bWarnedMissingComp = false;
};