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
enum class EWeaponGrip : uint8
{
	OneHand UMETA(DisplayName="One Hand"),   
	TwoHand UMETA(DisplayName="Two Hand"),   
	Melee   UMETA(DisplayName="Melee")       
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