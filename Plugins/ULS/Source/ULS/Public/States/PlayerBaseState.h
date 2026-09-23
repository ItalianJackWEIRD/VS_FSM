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
	
protected:
	virtual void OnJump();
	virtual void OnCrouch();
	void SetupDelegates();
	void ResetDelegates();
	
	void ApplyMovementParameters();
	const ULocomotionDataAsset* ResolveStateData() const;
	UPROPERTY()
	const ULocomotionDataAsset* StateData = nullptr;
	
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	
	UPROPERTY()
	ULocomotionStateComponent* LocoComp = nullptr;
	
	UPROPERTY()
	UVSCameraComponent* CameraRef = nullptr;
	
	
};
