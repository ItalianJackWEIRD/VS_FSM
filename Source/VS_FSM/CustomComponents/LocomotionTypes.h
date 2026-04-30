#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.generated.h"

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
	
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* R_01 = nullptr;
	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* L_02 = nullptr;
};