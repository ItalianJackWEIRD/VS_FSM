// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VS_FSMPlayerController.h"
#include "InputAction.h"
#include "CustomPlayerControllerInterface.h"
#include "CustomPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class VS_FSM_API ACustomPlayerController : public AVS_FSMPlayerController, public ICustomPlayerControllerInterface
{
	GENERATED_BODY()

	public:
		void DoJump();
	
	protected:
		virtual void SetupInputComponent() override;
		virtual FJumpSignature* GetJumpDelegate() override;

		UPROPERTY(EditAnywhere, Category = "Input")
		TObjectPtr<UInputAction> JumpAction;

	private:
		FJumpSignature JumpDelegate;
};
