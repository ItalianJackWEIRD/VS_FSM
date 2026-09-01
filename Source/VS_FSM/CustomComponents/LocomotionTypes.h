#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.generated.h"

UENUM(BlueprintType)
enum class  EStanceMode : uint8
{
	Normal,
	NormalRelaxed,
	Alert
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