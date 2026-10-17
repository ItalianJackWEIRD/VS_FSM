// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "VSCameraComponent.h"
#include "StateManagerComponent.h"
#include "GameFramework/Character.h"
#include "Components/LocomotionStateComponent.h"
#include "PlayerBaseState.generated.h"

class ULocomotionDataAsset;
class UCameraModeDataAsset;
class UVSCameraComponent;
/**
 * ho usato la forward declaration nel .h e ho usato l'include nel .cpp
 */
UCLASS()
class ULS_API UPlayerBaseState : public UStateBase
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY(BlueprintReadOnly)
	ACharacter* PlayerRef = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UStateManagerComponent* StateManager = nullptr;
	
	bool IsMoving() const;
	
	// PlayerBaseState.h — dove oggi hai il singolo StateData
	UPROPERTY(EditDefaultsOnly, Category="State Data", meta=(ToolTip="Se non ha più versioni, metterla su Normal"))
	TMap<EStanceMode, TObjectPtr<ULocomotionDataAsset>> State_StanceData;
	

	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;
	
	virtual void ApplyMovementParameters();
	
protected:
	virtual void OnJump();
	virtual void OnCrouch();
	void SetupDelegates();
	void ResetDelegates();
	
	
	const ULocomotionDataAsset* ResolveStateData() const;
	UPROPERTY()
	const ULocomotionDataAsset* StateData = nullptr;
	
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	
	UPROPERTY()
	ULocomotionStateComponent* LocoComp = nullptr;
	
	UPROPERTY()
	UVSCameraComponent* CameraRef = nullptr;
	
	// --- Comune a tutti gli stati che muovono il personaggio (prima stavano in ULocomotionState).
	//     Le sottoclassi le chiamano dal loro TickState, come prima.
	FVector GetIntendedDir();
	void PushOrientationDirection(FVector InSmoothedDir);
	virtual void UpdateOrientationDirection(float DeltaTime);
	void UpdateAnimationParameters(float DeltaTime);
	
	// Lean: 0 di default, così fuori dalla locomotion il lean torna a zero da solo.
	// ULocomotionState le sovrascrive con le formule vere.
	virtual float CalculateLeanFSM(float /*YawRate*/) const { return 0.f; }
	virtual float CalculateLeanMM(const FVector& /*Accel*/) const { return 0.f; }
	
	// Memoria per istanza: ogni stato ha la sua copia, quindi va inizializzata in OnEnterState.
	FVector PreviousVelocity = FVector::ZeroVector;
	float PreviousActorYaw = 0.f;
	
	
	// FALLING handle
	UFUNCTION()
	void HandleMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);
	
	// To override:
	virtual void OnStartFalling();
	virtual void OnLanded(const FHitResult& Hit) {}
	virtual bool WantsCrouchedCapsule() const { return false; }
};