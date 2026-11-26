#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "LocomotionTypes.generated.h"

UENUM(BlueprintType)
enum class  EStanceMode : uint8
{
	Normal,
	NormalRelaxed,
	Alert
};

UENUM(BlueprintType)
enum class EMovementGait : uint8
{
	Walk,
	Jog,
	Run
};

UENUM(BlueprintType)
enum class ELocomotionMode : uint8
{
	Ground,
	InAir,
	Slide,
	Traversal,
	Pipe
};

UENUM(BlueprintType)
enum class ELandType : uint8
{
	Light,
	Heavy,
	Roll
};

UENUM(BlueprintType)
enum class EOrientationDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

UENUM()
enum class ERootYawMode : uint8
{
	Accumulate,
	BlendOut
};

USTRUCT(BlueprintType)
struct FTwo_Anims
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* R_01 = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* L_02 = nullptr;
};

USTRUCT(BlueprintType)
struct FFour_Anims
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* F_01 = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* B_02 = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* L_03 = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* R_04 = nullptr;
};

USTRUCT(BlueprintType)
struct FLocomotion_Anims
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> F = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> B = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> L_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> L_R = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> R_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> R_R = nullptr;
};

USTRUCT(BlueprintType)
struct FCrouchLocomotion_Anims
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> F_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> F_R = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> B_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> B_R = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> L_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> L_R = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> R_L = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> R_R = nullptr;
};

USTRUCT(BlueprintType)
struct F2H_Anims
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* Stand = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* Crouch = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* Jog = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* WalkToJog = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* JogToWalk = nullptr;
};

/*
 * Pivot Data Structures
*/
USTRUCT(BlueprintType)
struct FPivotClip
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Anim = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0, ToolTip="Secondi da Saltare per far iniziare la animazione al punto di pivot."))
	float StartTime = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0, ToolTip="Secondi dopo il quale l'animazione esce e torna agli stati precedenti."))
	float EndTime = 0.f;
};

USTRUCT(BlueprintType)
struct FPivotFeet
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FPivotClip LFoot;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FPivotClip RFoot;
};

USTRUCT(BlueprintType)
struct FPivotShoulders
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="Varianti *L"))
	FPivotFeet L;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="Varianti *R. Lascia vuoto in Relaxed."))
	FPivotFeet R;
};

USTRUCT(BlueprintType)
struct FPivotDirections
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="F_B"))   FPivotFeet      FromForward;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="B_F"))   FPivotFeet      FromBackward;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="L*_R*")) FPivotShoulders FromLeft;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ToolTip="R*_L*")) FPivotShoulders FromRight;
};

/*
 * Traversal
*/
#pragma region TRAVERSAL
UENUM(BlueprintType)
enum class ETraversalActionType : uint8
{
	None,
	Hurdle UMETA(ToolTip="Scavalca un ostacolo sottile e atterra circa alla stessa quota (staccionata bassa)."),
	Vault  UMETA(ToolTip="Scavalca un ostacolo sottile e finisce in caduta (staccionata alta, o niente pavimento dietro)."),
	Mantle UMETA(ToolTip="Sale sopra l'ostacolo e ci resta."),
	DropDown UMETA(ToolTip="Scende sotto l'ostacolo e ci resta.")
};

// Input di CHT_TraversalAnims: solo i campi letti dalle colonne.
USTRUCT(BlueprintType)
struct FTraversalChooserInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasFrontLedge = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackLedge = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackFloor = false;
	/** Richiesta di drop-down (TryDropDown): le righe del traversal normale lo vogliono False. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDropDown = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Altezza del bordo anteriore sopra i piedi, in cm."))
	double ObstacleHeight = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Distanza 2D dal bordo anteriore a quello posteriore, o al punto d'urto sopra l'ostacolo, in cm."))
	double ObstacleDepth = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Altezza del bordo posteriore sopra il pavimento dietro, in cm."))
	double BackLedgeHeight = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EMovementMode> MovementMode = MOVE_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Modulo della Velocity 2D del CMC, in cm/s."))
	double Speed = 0.0;
};

// Output di CHT_TraversalAnims: la riga scelta nella tabella principale scrive il tipo d'azione.
USTRUCT(BlueprintType)
struct FTraversalChooserOutputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETraversalActionType ActionType = ETraversalActionType::None;
};

/** Drop-down automatico da un muretto: impostazioni nel DA degli stati che lo permettono (gli stati crouch). */
USTRUCT(BlueprintType)
struct ULS_API FDropDownSettings
{
	GENERATED_BODY()

	/** Controllo continuo: se acceso, lo stato prova il drop-down a ogni tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown")
	bool bEnabled = false;

	// --- Rilevamento.

	/** Sotto questo dislivello si scende camminando, senza montaggio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float MinDropHeight = 40.f;

	/** Oltre questo dislivello niente drop-down: si cade. Sotto, la chooser decide tra montaggio e High. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float MaxDropHeight = 300.f;

	/** Quanto avanti cercare lo spigolo, dal centro della capsula. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float SearchDistance = 120.f;

	/** Angolo massimo tra il movimento e la normale dello spigolo: oltre, lo spigolo è preso troppo di lato. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled", ClampMin="0.0", ClampMax="90.0"))
	float MaxApproachAngle = 50.f;

	// --- Innesco.

	/** Oltre questa distanza dallo spigolo si aspetta. Deve restare sotto l'inizio della clip (circa 105 cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float TriggerMaxDistance = 95.f;

	/** Sotto questa distanza parte subito, senza commit e anche da fermi: dal bordo non si cade e non ci si ferma. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float TriggerMinDistance = 25.f;

	/** Velocità minima per il commit nella finestra: più piano (passettini) il commit si azzera. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled"))
	float CommitMinSpeed = 60.f;

	/** Quanto a lungo il controllo deve riuscire di fila, nella finestra, prima di partire. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled", ClampMin="0.0"))
	float CommitTime = 0.2f;

	/** Sotto questa velocità si è fermi: nella finestra non parte, sotto la soglia minima sì, verso l'ultima direzione di marcia. */
	float StillSpeed = 10.f;
	
	// --- Atterraggio
	/** Spazio minimo tra la capsula e il muro sotto, a fine discesa. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled", ClampMin="0", Units="cm"))
	float LandingClearance = 10.f;

	/** Di quanto l'atterraggio può allontanarsi da quello della clip. Oltre, il warp deforma troppo: niente drop. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DropDown", meta=(EditCondition="bEnabled", ClampMin="0", Units="cm"))
	float MaxLandingPush = 40.f;
};
#pragma endregion

/** PIPE */
UENUM(BlueprintType)
enum class EPipeEnd : uint8 { Bottom, Top };

UENUM(BlueprintType)
enum class EPipePhase : uint8 { Entering, Hanging, Climbing, Exiting };
