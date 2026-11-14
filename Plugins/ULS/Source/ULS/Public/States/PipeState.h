// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "PipeState.generated.h"

class UPipeDataAsset;
class APipeActor;

/**
 * 
 */
UCLASS()
class ULS_API UPipeState : public UPlayerBaseState
{
	GENERATED_BODY()
	
public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;
	
protected:	
	virtual void OnCrouch() override;
	
	UPROPERTY()
	APipeActor* Pipe = nullptr;
	UPROPERTY()
	const UPipeDataAsset* PipeDataAsset = nullptr;
	
	bool bExitCrouched = false;
};
