// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionState.h"
#include "Kismet/KismetMathLibrary.h"
#include "CustomComponents/LocomotionTypes.h"
#include "PlayerBaseState.h"
#include "JogState.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API UJogState : public ULocomotionState
{
	GENERATED_BODY()
	
protected:
	virtual void OnJump() override;
	virtual void OnCrouch() override;
	
	virtual void OnEnterState(AActor* StateOwner) override;
	virtual void OnExitState() override;
	
	UPROPERTY(EditDefaultsOnly, Category="Camera Data")
	TObjectPtr<UCameraModeDataAsset> JogCameraData;
	
public:
	virtual void TickState(float DeltaTime) override;
	
private:
	void UpdateAnimationParameters(float DeltaTime);
	
};
