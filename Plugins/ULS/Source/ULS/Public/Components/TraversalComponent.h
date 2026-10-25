// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Types/LocomotionTypes.h"
#include "TraversalComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UCapsuleComponent;
class UCharacterMovementComponent;
class UChooserTable;
class UCustomAnimInstance;
class UMotionWarpingComponent;
class UPrimitiveComponent;
struct FHitResult;

/** Parametri della trace dell'ostacolo. Dipendono dal movement mode: valori del CBP di GASP. */
struct FTraversalCheckInputs
{
	FVector Direction = FVector::ForwardVector;
	float Distance = 0.f;
	FVector EndOffset = FVector::ZeroVector;
	float Radius = 30.f;
	float HalfHeight = 60.f;
};

/**
 * Esito di TryTraversal: la geometria dell'ostacolo e il montaggio scelto.
 * Lo stato traversal (Parte E) lo riceve e lo passa a PerformTraversal (Parte D5).
 */
USTRUCT(BlueprintType)
struct ULS_API FTraversalCheckResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	ETraversalActionType ActionType = ETraversalActionType::None;

	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	bool bHasFrontLedge = false;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	FVector FrontLedgeLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	FVector FrontLedgeNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	bool bHasBackLedge = false;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	FVector BackLedgeLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	FVector BackLedgeNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	bool bHasBackFloor = false;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	FVector BackFloorLocation = FVector::ZeroVector;

	/** Bordo anteriore sopra i piedi, in cm. */
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	float ObstacleHeight = 0.f;
	/** Distanza 2D dal bordo anteriore a quello posteriore, o al primo ingombro sopra l'ostacolo, in cm. */
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	float ObstacleDepth = 0.f;
	/** Bordo posteriore sopra il pavimento dietro, in cm. */
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	float BackLedgeHeight = 0.f;

	/** La componente dell'ostacolo: durante il montaggio la capsula la ignora (Parte D5). */
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	TObjectPtr<UAnimMontage> ChosenMontage = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	float StartTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Traversal")
	float PlayRate = 1.f;

	FString ToString() const;
};

/** Fine del traversal, completato o interrotto. Collisioni e movement mode sono già ripristinati. */
DECLARE_MULTICAST_DELEGATE_OneParam(FTraversalFinishedSignature, bool /*bInterrupted*/);

/**
 * Traversal di GASP (hurdle, vault, mantle) rifatto in C++ sopra i backend: uguale per FSM e MM.
 *
 * Rileva e sceglie, non decide. TryTraversal propone; la StateManager (UTraversalState, Parte E)
 * decide se entrare, e solo allora parte il montaggio (PerformTraversal, Parte D5).
 *
 * Parla solo con l'host (UCustomAnimInstance), mai con i layer di locomotion.
 * Niente rete: la replicazione del CBP di GASP e del port qui non c'è.
 */
UCLASS(ClassGroup=(Locomotion), meta=(BlueprintSpawnableComponent))
class ULS_API UTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTraversalComponent();

	/**
	 * Rilevamento (D3) e scelta (D4). Non fa partire niente.
	 * True se c'è un'azione valida: l'esito completo è in OutResult.
	 */
	bool TryTraversal(FTraversalCheckResult& OutResult);

	/** Perché l'ultima prova è fallita. Solo per il debug. */
	const FString& GetLastFailReason() const { return LastFailReason; }

	/**
	 * Esecuzione (D5), chiamata dallo stato: warp target, montaggio dal tempo e alla velocità scelti,
	 * capsula che ignora l'ostacolo, CMC in Flying. False se non è partito niente.
	 */
	bool PerformTraversal(const FTraversalCheckResult& Result);

	/** Uscita anticipata, chiamata dallo stato che esce: ferma il montaggio e ripristina. Non avvisa OnTraversalFinished. */
	void AbortTraversal();

	UFUNCTION(BlueprintCallable, Category="Traversal|Debug")
	void DebugTraversal();
	
	bool IsDoingTraversal() const { return bDoingTraversal; }

	/** Fine del montaggio, completato o interrotto. Lo ascolta lo stato traversal per uscire. */
	FTraversalFinishedSignature OnTraversalFinished;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** CHT_TraversalAnims: tipo d'azione e montaggi candidati. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal")
	TObjectPtr<UChooserTable> TraversalChooser = nullptr;

	/** Tag della Pose History nell'host: il Motion Match dei montaggi la cerca con questo nome. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal")
	FName PoseHistoryName = FName(TEXT("PoseHistory"));

	/** Attori o componenti con questo tag non si scavalcano. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal")
	FName NoTraversalTag = FName(TEXT("NoTraversal"));

	// --- Trace dell'ostacolo: valori del CBP di GASP.

	/** GASP usa un canale suo. Qui Visibility, finché non serve escludere oggetti: è un cambio di dati. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_Visibility;

	/** Canale delle sweep di spazio: fino al bordo, sopra l'ostacolo, pavimento dietro. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	TEnumAsByte<ECollisionChannel> RoomChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	float TraceRadius = 30.f;

	/** A terra la velocità in avanti (cm/s) si mappa sulla distanza della trace (cm). */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	FVector2D GroundSpeedRange = FVector2D(0.f, 500.f);
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	FVector2D GroundTraceDistanceRange = FVector2D(75.f, 350.f);
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	float GroundTraceHalfHeight = 60.f;

	/** In aria (Falling, Flying): distanza fissa, fine della trace alzata. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	float AirTraceDistance = 75.f;
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	float AirTraceEndOffsetZ = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Trace")
	float AirTraceHalfHeight = 86.f;

	// --- Bordi.

	/** Sotto questa larghezza è un palo, non un muretto. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Ledge")
	float MinLedgeWidth = 30.f;

	/** Z minima della normale della cima: sotto, è troppo inclinata per appoggiarsi (0.7 = circa 45 gradi). */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Ledge")
	float MinLedgeTopNormalZ = 0.7f;

	/** Senza retro (blocco profondo, terreno): quanto spazio cercare sopra. Da 59 in su la chooser sceglie Mantle. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Ledge")
	float MantleRoomDepth = 100.f;

	// --- Esecuzione: nomi dei warp target nei montaggi di GASP e della curva della distanza dal bordo.

	UPROPERTY(EditDefaultsOnly, Category="Traversal|Warp")
	FName FrontLedgeWarpName = FName(TEXT("FrontLedge"));
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Warp")
	FName BackLedgeWarpName = FName(TEXT("BackLedge"));
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Warp")
	FName BackFloorWarpName = FName(TEXT("BackFloor"));
	UPROPERTY(EditDefaultsOnly, Category="Traversal|Warp")
	FName DistanceFromLedgeCurveName = FName(TEXT("Distance_From_Ledge"));

	/** Blend out quando lo stato interrompe il traversal prima della fine. */
	UPROPERTY(EditDefaultsOnly, Category="Traversal")
	float AbortBlendOutTime = 0.2f;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Capsule = nullptr;
	/** Serve dalla Parte D5: warp target del montaggio. */
	UPROPERTY()
	TObjectPtr<UMotionWarpingComponent> MotionWarping = nullptr;

	UCustomAnimInstance* GetHostAnimInstance() const;

	bool RunCheck(FTraversalCheckResult& Result);
	FTraversalCheckInputs GetTraversalCheckInputs() const;
	bool IsTraversable(const FHitResult& Hit) const;
	bool ComputeLedges(const FHitResult& Hit, const FVector& TraceDirection, FTraversalCheckResult& Result) const;
	bool FindEdgeAlong(UPrimitiveComponent* Comp, const FVector& InsidePoint, const FVector& Dir, float Reach, FHitResult& OutHit) const;
	bool SweepCapsule(const FVector& Start, const FVector& End, float Radius, float HalfHeight, ECollisionChannel Channel, FHitResult& OutHit) const;
	bool ChooseAction(FTraversalCheckResult& Result) const;

	/** Segna il motivo e restituisce false: return Fail(TEXT("...")). */
	bool Fail(const TCHAR* Reason) const;

	// --- Esecuzione (D5-D6).
	void UpdateWarpTargets();
	bool GetCurveAtWarpWindowEnd(FName WarpTargetName, float& OutValue) const;
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void FinishTraversal(bool bInterrupted, bool bNotify);

	/** Il traversal in corso. UPROPERTY: tiene vivi montaggio e componente colpita. */
	UPROPERTY()
	FTraversalCheckResult ActiveResult;

	bool bDoingTraversal = false;
	TEnumAsByte<ECollisionResponse> SavedMeshWorldDynamicResponse = ECR_Block;

	// --- Debug: uls.traversal.DrawDebugLevel, DrawDebugDuration, AutoCheck.
	bool ShouldDrawDebug(int32 Level) const;
	float DebugLifeTime() const;
	void DrawLedges(const FTraversalCheckResult& Result) const;
	void DrawRoom(const FVector& Center, float Radius, float HalfHeight, bool bBlocked) const;

	mutable FString LastFailReason;
	FString LastLogKey;
	bool bAutoChecking = false;
};