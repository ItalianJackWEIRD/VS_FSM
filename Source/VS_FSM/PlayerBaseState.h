// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "VS_FSMCharacter.h"
#include "CustomComponents/CustomAnimInstance.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "PlayerBaseState.generated.h"

class ULocomotionDataAsset;
/**
 * ho usato la forward declaration nel .h e ho usato l'include nel .cpp
 */
UCLASS()
class VS_FSM_API UPlayerBaseState : public UStateBase
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY(BlueprintReadOnly)
	AVS_FSMCharacter* PlayerRef = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="State Data")
	TObjectPtr<ULocomotionDataAsset> StateData;
	
	ICustomPlayerControllerInterface* PlayerController = nullptr;
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	
protected:
	virtual void OnJump();
	virtual void OnCrouch();
	void SetupDelegates();
	void ResetDelegates();
	
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	
	UPROPERTY()
	UCustomAnimInstance* AnimInstance = nullptr;

};
