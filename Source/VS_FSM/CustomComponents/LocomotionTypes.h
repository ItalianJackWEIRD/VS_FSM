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
