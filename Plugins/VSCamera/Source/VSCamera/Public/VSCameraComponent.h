// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VSCameraComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCameraModeDataAsset;

UCLASS( ClassGroup=(Camera), meta=(BlueprintSpawnableComponent) )
class VSCAMERA_API UVSCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVSCameraComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraModeDataAsset> DefaultMode;
	
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraMode(UCameraModeDataAsset* Mode, bool bImmediate = false);
	
	// LEAN
	void SetLeanAngle(float Angle) { LeanInput = Angle; }
	float GetCurrentLeanRoll() const { return CurrentLeanRoll; }
 
protected:
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void CacheComponents();
	
	#pragma region LEAN
	
	float LeanInput = 0.0f;
	float CurrentLeanRoll = 0.0f;
	bool bModifiersRegistered = false;
	
	void RegisterLeanModifiers();
	
	UPROPERTY(Transient)
	TObjectPtr<class UVSCameraLeanModifier> LeanModifier;
	
	#pragma endregion
	
	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;
	
	UPROPERTY(Transient)
	TObjectPtr<UCameraModeDataAsset> TargetMode;

		
};
