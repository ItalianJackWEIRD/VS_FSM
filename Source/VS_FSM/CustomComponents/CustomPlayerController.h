// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VS_FSMPlayerController.h"
#include "InputAction.h"
#include "CustomPlayerControllerInterface.h"
#include "ShootingSystem.h"
#include "CustomPlayerController.generated.h"

struct FInputActionValue;
class AVS_FSMCharacter;
class UCustomAnimInstance;

enum class EStickInputSection : uint8 { Inner, Middle, Outer };

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
		void OnEquipPressed();
		void OnEquipReleased();
		void OnToggleWeapon();
		void OnChangeStance();
		void OnAimPressed();
		void OnAimReleased();
	
		virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
		virtual bool IsMovementInputZero() const override;
	
		// -- UI (for now, gonna be moved to another component)
		UPROPERTY(EditDefaultsOnly, Category="UI")
		TSubclassOf<UUserWidget> CrosshairWidgetClass;
		UPROPERTY()
		TObjectPtr<UUserWidget> CrosshairWidget = nullptr;
	
		UFUNCTION()
		void OnAimChangedUI(bool bAiming);
	
	protected:
		/* Soglie tunabili per controller */
		UPROPERTY(EditAnywhere, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
		float JogStickThreshold = 0.9f;
		UPROPERTY(EditAnywhere, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
		float DeadzoneThreshold = 0.1f;
		UPROPERTY(EditAnywhere, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0", ToolTip="Secondi al centro prima di considerarlo un rilascio vero e non un'inversione."))
		float CenterCommitTime = 0.12f;
		UPROPERTY(EditAnywhere, Category="Input", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0", ToolTip="Secondi in fascia walk prima di declassare da jog."))
		float WalkCommitTime = 0.15f;
	
		virtual void BeginPlay() override;
		virtual void PlayerTick(float DeltaTime) override;
	
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
		UPROPERTY(EditAnywhere, Category = "Input")
		TObjectPtr<UInputAction> ToggleEquipAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Move Input Action */
		UInputAction* MoveAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Look Input Action */
		UInputAction* LookAction;
		UPROPERTY(EditAnywhere, Category="Input")	/** Mouse Look Input Action */
		UInputAction* MouseLookAction;
		UPROPERTY(EditAnywhere, Category="Input")	
		UInputAction* ToggleWeapon;
		UPROPERTY(EditAnywhere, Category="Input")	
		UInputAction* ChangeStance;
		UPROPERTY(EditAnywhere, Category="Input")
		UInputAction* AimAction;
		
	
		void Move(const FInputActionValue& Value);
		void Look(const FInputActionValue& Value);
		void OnMoveCompleted(const FInputActionValue& Value);
	
		virtual void OnPossess(APawn* InPawn) override;
		virtual void OnUnPossess() override;

	private:
		void SetupInputActions(UEnhancedInputComponent* EIC);
	
		bool bMoveInputActive = false;
		bool bIsUsingController = false;
	
		/* Walk/Jog/Run */
		bool bToggleJogPressedExecuted = false;		// 	---> sarebbe bJogButtonHeld;
		float StickMagnitude = 0.f; // input vector con cui confrontare contro la velocità
		float CenterTimer = 0.f;
		float WalkTimer = 0.f; 
		EStickInputSection StickSection = EStickInputSection::Inner; // Inner means basically No-input.
	
		void ResolveGait(float DeltaTime);
		void ReloadStickTimers();
		void CaptureMovStopSnapshot();
	
		UPROPERTY()
		TObjectPtr<AVS_FSMCharacter> PlayerCharacter = nullptr;
		UPROPERTY()
		TObjectPtr<UCustomAnimInstance> CustomAnimInstance = nullptr;
		UPROPERTY()
		TObjectPtr<UShootingSystem> ShootingComponent = nullptr;
	
		FJumpSignature JumpDelegate;
		FCrouchSignature CrouchDelegate;
};
