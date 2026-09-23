// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/LocomotionTypes.h"
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
 * REGOLA: il grafo non tocca MAI il componente. Lavora su una copia propria, e il
 * C++ fa da traghetto sul game thread, in NativeUpdateAnimation:
 *   1. SyncFeedback()      — porta al componente ciò che il grafo ha scritto o consumato
 *   2. PullFromComponent() — copia lo stato aggiornato nei membri letti dal grafo
 *
 * Proprietà dei dati, imposta dai tipi:
 *   - ciò che il grafo POSSIEDE (bAnimGraphIn*) lo scrive con un Set;
 *   - ciò che possiede il C++ il grafo lo legge e basta. Può solo CONSUMARLO,
 *     con le funzioni Clear* e Should*: rimetterlo a true è impossibile.
 *
 * LocoComp non è esposto al Blueprint: se un nodo del layer prova a usarlo,
 * non compila. È voluto.
 */
UCLASS()
class ULS_API ULocoFSMAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

	/*--- Serve alle transition rule che devono restare addormentate per sempre ---*/
	UPROPERTY(BlueprintReadOnly)
	bool bAlwaysFalse = false;

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

#pragma region ANIMGRAPH FEEDBACK

	// --- Del GRAFO: le scrive solo lui, con un Set. Vanno al componente ogni frame.
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInIdle = false;
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInMovStop = false;
	UPROPERTY(BlueprintReadWrite, Category="General ABP Settings")
	bool bAnimGraphInRunStop = false;

	// --- Del C++: il C++ le arma, il grafo le legge e può solo consumarle via Clear*.
	//     ReadOnly apposta: dal grafo non si possono rimettere a true.
	UPROPERTY(BlueprintReadOnly, Category="Locomotion-Pivot")
	bool bShouldPivot = false;
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	bool bShouldRecenterIdle = false;
	UPROPERTY(BlueprintReadOnly, Category="Idle")
	bool bIsIdleBreak = false;

	UFUNCTION(BlueprintCallable, Category="Locomotion-Pivot", meta=(BlueprintThreadSafe))
	void ClearShouldPivot();
	UFUNCTION(BlueprintCallable, Category="Idle", meta=(BlueprintThreadSafe))
	void ClearShouldRecenterIdle();
	UFUNCTION(BlueprintCallable, Category="Idle", meta=(BlueprintThreadSafe))
	void ClearIsIdleBreak();

	// --- Trigger one-shot del C++: leggerli li consuma.
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldIdleBreak();
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	bool ShouldMovWalkJogStanceTransition();

#pragma endregion

	/** Copia locale: niente puntatori esterni dereferenziati da un worker thread. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	EStanceMode GetStanceMode() const { return StanceMode; }

	// --- Le notify vengono dispatchate sul game thread: qui scrivere sul componente è lecito.
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EStanceMode StanceMode = EStanceMode::Normal;

private:
	/** NON esposto al Blueprint: il grafo del layer non deve poterlo raggiungere. */
	UPROPERTY()
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	bool EnsureLocoComp();
	void SyncFeedback();
	void PullFromComponent();

	/** Warning una volta sola: EnsureLocoComp gira ogni frame. */
	bool bWarnedMissingComp = false;

	// Consumi fatti dal grafo, in attesa di arrivare al componente.
	bool bPendingClearShouldPivot = false;
	bool bPendingClearShouldRecenterIdle = false;
	bool bPendingClearIsIdleBreak = false;

	// Trigger one-shot: copie locali e relativi consumi in attesa.
	bool bIdleBreakTrigger = false;
	bool bStanceTransitionTrigger = false;
	bool bWalkJogTransitionTrigger = false;
	bool bPendingConsumeIdleBreak = false;
	bool bPendingConsumeStanceTransition = false;
	bool bPendingConsumeWalkJogTransition = false;
};