// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CameraModeDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class VSCAMERA_API UCameraModeDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
	public:
	// --- Arm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Arm")
	float TargetArmLength = 280.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Arm")
	FVector SocketOffset = FVector(0, 65.f, 35.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Arm")
	FVector TargetSocket = FVector(0, 15.f, 10.f);
	
	// --- Lens
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lens")
	float FieldOfView = 76.f;
	
	// --- Lag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag")
	bool bEnableCameraLag = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag")
	bool bEnableCameraRotationLag = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta=(EditCondition="bEnableCameraLag"))
	float CameraLagSpeed = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta=(EditCondition="bEnableCameraRotationLag"))
	float CameraRotationLagSpeed = 12.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta=(EditCondition="bEnableCameraLag"))
	float CameraLagMaxDistance = 0.f;
	
	// --- Collision (future)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Collision")
	bool bDoCollisionTest = true;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Collision")
	float ProbeSize = 12.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Blend", meta=(ClampMin="0.0"))
	float BlendInterpSpeed = 10.f;
	
	// --- Lean 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Lean")
	float LeanMultiplier = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Lean", meta=(ClampMin="0.0"))
	float MaxLeanAngle = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Lean", meta=(ClampMin="0.0"))
	float LeanInterpSpeed = 7.f;
};
