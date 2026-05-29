// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VS_FSMPlayerController.h"
#include "InputAction.h"
#include "CustomPlayerControllerInterface.h"
#include "CustomPlayerController.generated.h"

struct FInputActionValue;
class AVS_FSMCharacter;
class UCustomAnimInstance;

/**
 * 
 */
UCLASS()
class VS_FSM_API ACustomPlayerController : public AVS_FSMPlayerController, public ICustomPlayerControllerInterface
{
	GENERATED_BODY()

	public:
		void DoJump();
		void DoCrouch();
		void OnJogPressed();
		void OnJogReleased();
	
		virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
		virtual bool IsMovementInputZero() const override;
	
	protected:
		UPROPERTY(EditAnywhere, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
		float JogStickThreshold = 0.9f;
	
		virtual void BeginPlay() override;
	
		UFUNCTION()
		void OnInputHardwareChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId);
	
		virtual void SetupInputComponent() override;
		virtual FJumpSignature* GetJumpDelegate() override;
		virtual FCrouchSignature* GetCrouchDelegate() override;

		UPROPERTY(EditAnywhere, Category = "Input")
		TObjectPtr<UInputAction> JumpAction;
		UPROPERTY(EditAnywhere, Category = "Input")
		TObjectPtr<UInputAction> CrouchAction;
		UPROPERTY(EditAnywhere, Category = "Input")
		TObjectPtr<UInputAction> ToggleJogAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Move Input Action */
		UInputAction* MoveAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Look Input Action */
		UInputAction* LookAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Mouse Look Input Action */
		UInputAction* MouseLookAction;
	
		void Move(const FInputActionValue& Value);
		void Look(const FInputActionValue& Value);
		void OnMoveCompleted(const FInputActionValue& Value);
	
		virtual void OnPossess(APawn* InPawn) override;
		virtual void OnUnPossess() override;

	private:
		void SetupInputActions(UEnhancedInputComponent* EIC);
	
		bool bMoveInputActive = false;
		bool bIsUsingController = false;
	
		UPROPERTY()
		TObjectPtr<AVS_FSMCharacter> PlayerCharacter = nullptr;
		UPROPERTY()
		TObjectPtr<UCustomAnimInstance> CustomAnimInstance = nullptr;
	
		FJumpSignature JumpDelegate;
		FCrouchSignature CrouchDelegate;
};
