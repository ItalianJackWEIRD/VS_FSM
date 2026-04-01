// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "VS_FSMCharacter.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "PlayerBaseState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UPlayerBaseState : public UStateBase
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY(BlueprintReadOnly)
	AVS_FSMCharacter* PlayerRef = nullptr;
	
	ICustomPlayerControllerInterface* PlayerController = nullptr;
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	
protected:
	virtual void OnJump();
};
