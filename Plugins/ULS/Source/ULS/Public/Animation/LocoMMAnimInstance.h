// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/LocomotionTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/TrajectoryTypes.h"
#include "LocoMMAnimInstance.generated.h"

class ULocomotionStateComponent;

/**
 * Parent class di ABP_Loco_MM, il layer di locomotion Motion Matching.
 *
 * Sorella di ULocoFSMAnimInstance, non figlia: deriva da UAnimInstance puro e non
 * conosce né la FSM né il character.
 *
 * Stessa regola della FSM: il grafo non tocca MAI il componente. Il C++ copia il
 * contratto nei membri in NativeUpdateAnimation, sul game thread, e calcola qui le
 * condizioni della chooser (IsMoving, IsStarting, IsPivoting, ShouldTurnInPlace),
 * come le funzioni dell'ABP di GASP. Chooser e nodo MM leggono solo questi membri,
 * anche dal worker thread.
 *
 * Differenze rispetto alla FSM:
 *   - niente SyncFeedback: l'unica cosa che il grafo MM scrive sono i tag del DB
 *     scelto (CurrentDatabaseTags), e restano dentro questa instance.
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

	/** Colonna Speed 2D della chooser (Stand Idles). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	float Speed2D = 0.f;

	/** Accelerazione live dal CMC, copiata ogni frame. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	FVector Acceleration = FVector::ZeroVector;

	/** Direzione per l'Orientation Warping: resta valida anche da fermi. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	FVector LastNonZeroVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	bool bIsAiming = false;

	/** Colonna Gait della chooser. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	EMovementGait MovementGait = EMovementGait::Walk;

	/** Colonna della chooser: Alert / Normal. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Contract")
	EStanceMode StanceMode = EStanceMode::Normal;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|PlayRate")
	float PlayRate = 1.f;

#pragma endregion

#pragma region ORIENTATION_LEAN

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Orientation")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Lean")
	float LeanAngle = 0.f;

#pragma endregion

#pragma region TRAJECTORY

	/** Copia della traiettoria del componente: la legge Get_DesiredFacing per lo Steering. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Trajectory")
	FTransformTrajectory Trajectory;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Trajectory")
	FVector TrjPastVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Trajectory")
	FVector TrjCurrentVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Trajectory")
	FVector TrjFutureVelocity = FVector::ZeroVector;

#pragma endregion

#pragma region SELECTION	// colonne della chooser e interruzione del MM, come in GASP

	/** Movement State della chooser: c'è input e la traiettoria prevede di muoversi. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|MotionMatching")
	bool bIsMoving = false;

	/** La velocità futura supera l'attuale: si sta partendo. Mai durante un pivot. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|MotionMatching")
	bool bIsStarting = false;

	/** La direzione futura si stacca da quella attuale oltre PivotAngleThreshold. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|MotionMatching")
	bool bIsPivoting = false;

	/** L'osso root si è staccato dalla capsula oltre TurnInPlaceAngleThreshold. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|MotionMatching")
	bool bShouldTurnInPlace = false;

	/** True solo nel frame in cui cambia la categoria della chooser: fermo/in movimento, o gait in movimento. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|MotionMatching")
	bool bDatabaseCategoryChanged = false;

	/**
	 * Tag del DB della posa scelta. Li scrive Update_MotionMatching_PostSelection (Blueprint),
	 * li leggono IsStarting qui e lo Steering del turn in place nel Blend Stack.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	TArray<FName> CurrentDatabaseTags;

	/** GASP: 30 in strafe, 45 con orient to movement. Il personaggio di ULS è sempre in strafe, per ora. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|MotionMatching")
	float PivotAngleThreshold = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|MotionMatching")
	float TurnInPlaceAngleThreshold = 50.f;

#pragma endregion

private:
	/** NON esposto al Blueprint, come nella FSM: il grafo lavora solo sulle copie. */
	UPROPERTY()
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	bool EnsureLocoComp();
	void PullFromComponent();
	void UpdateSelectionConditions();
	
	float GetTrajectoryTurnAngle() const;

	/** Warning una volta sola: EnsureLocoComp gira ogni frame. */
	bool bWarnedMissingComp = false;

	bool bPrevIsMoving = false;
	EMovementGait PrevGait = EMovementGait::Walk;
};
	
