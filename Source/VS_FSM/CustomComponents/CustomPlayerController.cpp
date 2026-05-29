// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomPlayerController.h"
#include "CustomComponents/CustomAnimInstance.h"
#include "VS_FSMCharacter.h"
#include "InputActionValue.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "EnhancedInputComponent.h"

void ACustomPlayerController::DoJump()
{
	if(JumpDelegate.IsBound())
		JumpDelegate.Broadcast();
}

void ACustomPlayerController::DoCrouch()
{
	if (CrouchDelegate.IsBound())
		CrouchDelegate.Broadcast();
}

void ACustomPlayerController::OnJogPressed()
{
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
	{
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->bIsJogging;
		CustomAnimInstance->bIsRunning = true;
	}
	else // per ora else, perche abbiamo solo normal e normalRelaxed, quando saranno di piu cambia con else if
	{
		CustomAnimInstance->bIsJogging = true;
	}
}

void ACustomPlayerController::OnJogReleased()
{
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
	{
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->bIsJogging; // cosi ABP tiene traccia per i cambi
		CustomAnimInstance->bIsRunning = false;
	}
	else
	{
		CustomAnimInstance->bIsJogging = false;
		CustomAnimInstance->bIsRunning = false; // fallback nel caso entriamo Alert e usciamo Normal, nel caso viceversa non si rompe niente perche torniamo in jogging normale
	}
}

void ACustomPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (UInputDeviceSubsystem* InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>())
	{
		InputDeviceSubsystem->OnInputHardwareDeviceChanged.AddDynamic(this, &ACustomPlayerController::OnInputHardwareChanged);

		if (const ULocalPlayer* LP = GetLocalPlayer())
		{
			const FHardwareDeviceIdentifier Device = InputDeviceSubsystem->GetMostRecentlyUsedHardwareDevice(LP->GetPlatformUserId());
			bIsUsingController = Device.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad;
		}
	}
}

void ACustomPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInputDeviceSubsystem* InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>())
	{
		InputDeviceSubsystem->OnInputHardwareDeviceChanged.RemoveDynamic(this, &ACustomPlayerController::OnInputHardwareChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void ACustomPlayerController::OnInputHardwareChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId)
{
	if (UInputDeviceSubsystem* InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>())
	{
		const FHardwareDeviceIdentifier Device = InputDeviceSubsystem->GetMostRecentlyUsedHardwareDevice(UserId);
		bIsUsingController = Device.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad;
		// Leva Debug
		GEngine->AddOnScreenDebugMessage(7, 2.f,
				bIsUsingController ? FColor::Green : FColor::Orange,
				FString::Printf(TEXT("Input device changed -> %s"),
					bIsUsingController ? TEXT("GAMEPAD") : TEXT("KBM")));
	}
}

void ACustomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind input actions and axes here
	if(UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Example: Bind the "Jump" action to the DoJump function
		SetupInputActions(EIC);
	}
}

bool ACustomPlayerController::IsMovementInputZero() const
{
	return !bMoveInputActive;
}

void ACustomPlayerController::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	bMoveInputActive = !MovementVector.IsNearlyZero();
	
	if (!PlayerCharacter) return;	
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert && bIsUsingController)
	{
		CustomAnimInstance->bIsJogging = MovementVector.Size() >= JogStickThreshold;
		
		const FVector2D MaxInput = MovementVector.GetSafeNormal();
		PlayerCharacter->DoMove(MaxInput.X, MaxInput.Y);
		return;
	}
	
	PlayerCharacter->DoMove(MovementVector.X, MovementVector.Y);
}

void ACustomPlayerController::Look(const FInputActionValue& Value)
{
	if (!PlayerCharacter) return;
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	PlayerCharacter->DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACustomPlayerController::OnMoveCompleted(const FInputActionValue& Value)
{
	bMoveInputActive = false;
}

void ACustomPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	PlayerCharacter = Cast<AVS_FSMCharacter>(InPawn);
	if (PlayerCharacter && PlayerCharacter->GetMesh())
		CustomAnimInstance = Cast<UCustomAnimInstance>(PlayerCharacter->GetMesh()->GetAnimInstance());
	else
		UE_LOG(LogTemp, Warning, TEXT("CustomAnimInstance cast skipped: move it inside the function where you actually need it, don't cast upfront"));
}

void ACustomPlayerController::OnUnPossess()
{
	PlayerCharacter = nullptr;
	CustomAnimInstance = nullptr;
	Super::OnUnPossess();
}

void ACustomPlayerController::SetupInputActions(UEnhancedInputComponent* EIC)
{
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoJump);
	EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoCrouch);
	EIC->BindAction(ToggleJogAction, ETriggerEvent::Started, this, &ACustomPlayerController::OnJogPressed);
	EIC->BindAction(ToggleJogAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnJogReleased);
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Move);
	EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnMoveCompleted);
	EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ACustomPlayerController::OnMoveCompleted);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Look);
	EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Look);
}

FJumpSignature* ACustomPlayerController::GetJumpDelegate()
{
	return &JumpDelegate;
}

FCrouchSignature* ACustomPlayerController::GetCrouchDelegate()
{
	return &CrouchDelegate;
}
