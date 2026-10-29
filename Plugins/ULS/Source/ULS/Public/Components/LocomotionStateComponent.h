// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/TrajectoryTypes.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Types/LocomotionTypes.h"
#include "Types/IkContextTypes.h"
#include "LocomotionStateComponent.generated.h"

class UAnimSequence;
class UCharacterMovementComponent;

DECLARE_MULTICAST_DELEGATE(FStanceChangedSignature);
DECLARE_MULTICAST_DELEGATE(FCrouchRequestedSignature);
DECLARE_MULTICAST_DELEGATE(FJumpRequestedSignature);

UENUM(BlueprintType)
enum class ELocomotionBackend : uint8 { MM , FSM, Custom };

/**
 * Contenitore dei dati di locomotion — "lo zaino che il character indossa".
 *
 * Questo file non include e non nomina NESSUN AnimInstance: è esattamente ciò che
 * rende i due backend (FSM / Motion Matching) intercambiabili. Se un giorno ti
 * ritrovi a scrivere qui dentro "CustomAnimInstance", il refactor è già rotto.
 *
 * È anche l'unico oggetto che ABP_Host e il layer di locomotion vedono ENTRAMBI:
 * sono due UAnimInstance distinti a runtime, non condividono una riga di stato.
 * Tutto ciò che deve passare fra i due passa da qui.
 *
 * Blocchi:
 *   1. CONTRATTO   — sempre attivo, letto da qualunque backend
 *   2. SPRINT      — soglie del gate, isteresi esplicita
 *   3. BACKEND FSM — spento dai flag bEnable*, GASP non lo guarda
 *   4. FEEDBACK    — scritto dall'AnimGraph, letto dal gameplay (l'unico blocco ReadWrite)
 *   5. SET ANIM    — configurazione del backend FSM, letta dal C++ per scegliere le clip
 *
 * Regola del single writer: ogni variabile ha un solo proprietario. I blocchi 1-3 e 5
 * li scrive il C++ e il grafo li legge; il blocco 4 è l'opposto. Mai i due insieme.
 */
UCLASS(ClassGroup=(Locomotion), meta=(BlueprintSpawnableComponent))
class ULS_API ULocomotionStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULocomotionStateComponent();
	
	bool IsFSMBackend() const { return LocomotionType == ELocomotionBackend::FSM; }	// refattorizza in bool dedicati per chi usa questa chiamata Generale.

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	/** Unico interruttore del backend: flag, reset dello stato FSM, layer linkato. */
	UFUNCTION(BlueprintCallable, Category="Locomotion|Backend")
	void SetLocomotionLayer(ELocomotionBackend NewType);
	
	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Backend")
	TSubclassOf<UAnimInstance> FSMLayerClass;
	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Backend")
	TSubclassOf<UAnimInstance> MMLayerClass;
	
	
// ============================================================================
// 1. CONTRATTO — letto da entrambi i backend. Nessun flag lo spegne.
// ============================================================================
#pragma region CONTRACT

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	FVector VelocityXY = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	float LandSpeed = 0.f;

	/** Intento di movimento da input (Enhanced Input), non velocità fisica. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	bool bShouldMove = false;
	
	/** Input di movimento a zero, con l'isteresi dello stick. Lo scrive il controller. Da confrontare sul fronte MovStop con bShouldMove */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	bool bIsMovementInputZero = true;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	bool bIsAiming = false;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	bool bIsCrouched = false;
	
	FCrouchRequestedSignature CrouchRequestedDelegate;
	
	UFUNCTION(BlueprintCallable, Category="Locomotion|Contract")
	void RequestCrouch() { CrouchRequestedDelegate.Broadcast(); }

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	EMovementGait MovementGait = EMovementGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	ELocomotionMode LocomotionMode = ELocomotionMode::Ground;
	
	/** Specchio di AVS_FSMCharacter::StanceMode. Lo scrive SetStanceMode(), non la FSM. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	EStanceMode StanceMode = EStanceMode::Normal;
	
	FStanceChangedSignature StanceChangedDelegate;
	
	FJumpRequestedSignature JumpRequestedDelegate;
	
	UFUNCTION(BlueprintCallable, Category="Locomotion|Contract")
	void RequestJump() { JumpRequestedDelegate.Broadcast(); }

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Contract")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;

	/** Accelerazione live dal CMC. Non cachata: GASP la vuole fresca per la trajectory. */
	UFUNCTION(BlueprintPure, Category="Locomotion|Contract")
	FVector GetAcceleration() const;
	
#pragma region AIR AND LAND
	
	ELandType LandType = ELandType::Light;
	
	/** Frenata d'atterraggio (FSM): accesa da InAir all'uscita, spenta dal componente dopo LandBrakingDuration. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Land Braking")
	bool bLandBraking = false;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Land Braking", meta=(ClampMin="0"))
	float LandBrakingDuration = 2.f;

	void StartLandBraking() { bLandBraking = true; LandBrakingElapsed = 0.f; }
	
#pragma endregion
	
#pragma region TRAVERSAL
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Traversal")
	bool bMontageActive = false;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Traversal")
	float MovingTraversalCurveValue = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Traversal")
	bool bTraversalEndsOnTop = false;
	
#pragma endregion 
	
#pragma region ORIENTATION_WARPING

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	EOrientationDirection OrientationDirection = EOrientationDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	float OrientationAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	FVector SmoothedDir = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	float Fwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	float Bwd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	float Left = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Orientation")
	float Right = 0.f;

#pragma endregion

#pragma region TRAJECTORY
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Trajectory")
	FTransformTrajectory Trajectory;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	FPoseSearchTrajectoryData TrajectoryDataIdle;
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	FPoseSearchTrajectoryData TrajectoryDataMoving;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	float HistorySamplingInterval = -1.f;
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	int32 HistoryCount = 30;
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	float PredictionSamplingInterval = 0.1f;
	UPROPERTY(EditAnywhere, Category="Locomotion|Trajectory")
	int32 PredictionCount = 15;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Trajectory")
	FVector TrjPastVelocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Trajectory")
	FVector TrjCurrentVelocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Trajectory")
	FVector TrjFutureVelocity = FVector::ZeroVector;
	
	
#pragma endregion
	
#pragma endregion

// ============================================================================
// 2. BACKEND MM
//    Tutti i dati che servono SOLO al Motion Matching
// ============================================================================	
#pragma region LEAN_MM 
	
	UPROPERTY(BlueprintReadOnly, Category="LocomotionMM|Lean")
	float LeanAngleMM = 0.f;
	
	float TargetLeanAngleMM = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category="LocomotionMM|Lean")
	float LeanMMInterpSpeed = 4.f;
	
#pragma endregion
	
// ============================================================================
// 3. BACKEND FSM — la prima Region offre dei flag che spengono le varie parti 
//   del BE. Tutti i dati che servono SOLO alla FSM
// ============================================================================
#pragma region FSM_BACKEND
	
#pragma region FSM_FLAGS
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Locomotion|Backend FSM")
	ELocomotionBackend LocomotionType = ELocomotionBackend::FSM;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableOrientationWarping = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnablePivot = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableShoulderVariants = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableTurnInPlace = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableIdleBreak = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableIdleRecenter = true;

	UPROPERTY(EditAnywhere, Category="Locomotion|Backend FSM")
	bool bEnableDistanceMatching = true;

#pragma endregion

#pragma region LEAN	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Lean")
	float LeanAngle = 0.f;

	/** 0 CrouchWalk, 1 Idle/Walk, 2 Jog, 3 Run — indice per il BlendSpace dei lean. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Lean")
	int32 LeanStateIndex = 0;

#pragma endregion

#pragma region PIVOT	// flag: bEnablePivot

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Pivot")
	TObjectPtr<UAnimSequence> PivotAnim = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Pivot")
	float PivotStartTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Pivot")
	float PivotEndTimeRemaining = 0.f;

#pragma endregion

#pragma region SHOULDER	// flag: bEnableShoulderVariants 

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Shoulder")
	bool bLeftShoulderLocomotion = false;

#pragma endregion

#pragma region TURN_IN_PLACE	// flag: bEnableTurnInPlace 

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Turn")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Turn")
	bool bShouldTurnLeft = false;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Turn")
	bool bShouldTurnRight = false;

	/** Solo C++ — all'AnimGraph non serve. */
	ERootYawMode RootYawMode = ERootYawMode::Accumulate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Locomotion|Turn",
		meta=(ToolTip="Gradi di RootYawOffset oltre i quali parte il turn in place. Riporta il valore che avevi nelle Class Defaults dell'ABP."))
	float TurnThreshold = 50.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Turn")
	float TurnAnimElapsedTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Turn")
	TObjectPtr<UAnimSequence> FinalTurnAnim = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Turn")
	FString TurnYawCurveName = FString(TEXT("TurnYawWeight"));

#pragma endregion

#pragma region IDLE	// flag: bEnableIdleBreak / bEnableIdleRecenter

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Idle")
	TObjectPtr<UAnimSequence> FinalIdleAnim = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Idle")
	TObjectPtr<UAnimSequence> FinalIdleBreakAnim = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Idle")
	TObjectPtr<UAnimSequence> FinalIdleRecenterAnim = nullptr;

	/** Trigger armato dalla FSM, consumato da ShouldIdleBreak(). Solo C++. */
	bool bShouldIdleBreak = false;

#pragma endregion

#pragma region STANCE_TRANSITION	// gameplay — no flag

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Stance Transition")
	TObjectPtr<UAnimSequence> FinalStanceTransitionAnim = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Stance Transition")
	bool bTransitionRunInJog = false;

	/** Trigger consumato da ShouldStanceTransition(). */
	bool bShouldStanceTransition = false;
	/** Guardia, chiusa da ResetStanceTransition() via notify + watchdog a 3s nella FSM. */
	bool bIsInStanceTransition = false;
	float StanceTransitionStartTime = 0.f;

	/** Trigger consumato da ShouldMovWalkJogStanceTransition(). */
	bool bShouldWalkJogStanceTransition = false;
	bool bIsInWalkJogStanceTransition = false;
	float WalkJogTransitionStartTime = 0.f;

#pragma endregion

#pragma region MOV_STOP	// gameplay — no flag

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|MovStop")
	bool bMovStopJogging = false;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|MovStop")
	bool bMovStopCrouched = false;

	UPROPERTY(EditAnywhere, Category="Locomotion|MovStop", meta=(
		ToolTip="Settala a metà strada tra la MaxWalkSpeed di walk e quella di jog",
		ClampMin="0.0", ClampMax="1000.0", UIMin="0.0", UIMax="1000.0"))
	float MovStopJogSpeedThreshold = 350.f;

#pragma endregion

#pragma region PLAY_RATE

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|PlayRate")
	float PlayRate = 1.f;

	/** Target del DataAsset, interpolato verso PlayRate. Solo C++. */
	float TargetPlayRate = 1.f;

#pragma endregion

#pragma region DISTANCE_MATCHING	// flag: bEnableDistanceMatching 

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	bool bUseSeparateBrakingFriction = false;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	float BrakingFriction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	float GroundFriction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	float BrakingFrictionFactor = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	float BrakingDecelerationWalking = 0.f;

	/** 30 di default, cambiata solo su Enter/Exit di AimState. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion|Distance Matching")
	float MinDistanceToDistanceMatch = 30.f;

	/** Ricarica la cache dal CMC. Da chiamare dopo ogni ApplyMovementParameters(). */
	UFUNCTION(BlueprintCallable, Category="Locomotion|Distance Matching")
	void RefreshMovementCache();

#pragma endregion
	
#pragma endregion
	
// ============================================================================
// 4. IK CONTESTUALI - Sistema di Movimenti contestuali governati da blend 
//   additivi o IK. Ogni meccanica ha il suo flag per essere spenta.
// ============================================================================
#pragma region Ik_Context
	
#pragma region FLAGS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IkContext|Flags")
	bool bEnableShoulderAvoidance = true;
#pragma endregion
	
#pragma region AVOIDANCE
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|AVOIDANCE")
	bool bShoulderAvoidanceOnState = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|AVOIDANCE")
	bool bShoulderAvoidanceLeanState = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|AVOIDANCE")
	float ShoulderAvoidanceAlpha = 0.f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|AVOIDANCE", meta=(tooltip="0 spento, 1 dx, -1 sx"))
	float ShoulderAvoidanceWeight = 0.f;
	
#pragma endregion
	
#pragma region WALL_TOUCH
	
	/** Wall touch on/off for this project. Set in the Details. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IkContext|WallTouch")
	bool bEnableWallTouch = true;

	/** Written by the states from their DA: the current state allows the wall touch. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|WallTouch")
	bool bWallTouchOnState = false;

	/** Arm channels: written only by UIkContextComponent, copied by the host. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|Arms")
	FIkArmChannel ArmL;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext|Arms")
	FIkArmChannel ArmR;
	
#pragma endregion
	
#pragma endregion
	
// ============================================================================
// 5. FEEDBACK DALL'ANIMGRAPH — l'unico blocco che scrive il grafo.
//    Il layer ci arriva col suo riferimento al componente: non serve che
//    conosca nessuna classe C++ di AnimInstance.
// ============================================================================
#pragma region ANIMGRAPH_FEEDBACK

	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bAnimGraphInIdle = false;

	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bAnimGraphInMovStop = false;

	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bAnimGraphInRunStop = false;

	/** Armato da CheckPivot(), rimesso a false dal grafo quando consuma il pivot. */
	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bShouldPivot = false;

	/** Calcolato al rilascio dell'input, resettato dal grafo all'ingresso in idle. */
	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bShouldRecenterIdle = false;

	UPROPERTY(BlueprintReadWrite, Category="Locomotion|AnimGraph Feedback")
	bool bIsIdleBreak = false;

	// --- Trigger one-shot: la FSM arma, il grafo consuma leggendo.
	UFUNCTION(BlueprintPure, Category="Locomotion|AnimGraph Feedback", meta=(BlueprintThreadSafe))
	bool ShouldIdleBreak();

	UFUNCTION(BlueprintPure, Category="Locomotion|AnimGraph Feedback", meta=(BlueprintThreadSafe))
	bool ShouldStanceTransition();

	UFUNCTION(BlueprintPure, Category="Locomotion|AnimGraph Feedback", meta=(BlueprintThreadSafe))
	bool ShouldMovWalkJogStanceTransition();

	// --- Chiusura delle guardie, chiamata dalle notify.
	UFUNCTION(BlueprintCallable, Category="Locomotion|AnimGraph Feedback")
	void ResetStanceTransition();		// se il crouch si rompe in idle è per colpa di questo


#pragma endregion

// ============================================================================
// 6. SET DI ANIMAZIONI — configurazione del backend FSM.
//    Letti dal C++ per scegliere la clip. Chi usa GASP li lascia vuoti.
// ============================================================================
#pragma region ANIM_SETS

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Idle")
	FFour_Anims IdleBreakAnims;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Idle")
	FTwo_Anims IdleAnims;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Idle")
	FTwo_Anims IdleRecenterAnims;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Idle")
	FTwo_Anims IdleCrouchRecenterAnims;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Turn")
	FTwo_Anims TurnAnimsStanding;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Turn")
	FTwo_Anims TurnAnimsCrouching;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Anims|Turn")
	FTwo_Anims StanceTransitionAnims;

#pragma endregion
	
	void ResetFSMTransientState();
	
	
private:
	/** Backend */
	void ApplyBackendFlags();
	
	/** TRAJECTORY */
	FTransformTrajectory TrajectoryState;
	float DesiredControllerYawLastUpdate = 0.f;
	
	void UpdateTrajectory(float DeltaTime);
	
	void LandBrakingTimer(float DeltaTime);
	float LandBrakingElapsed = 0.f;
};