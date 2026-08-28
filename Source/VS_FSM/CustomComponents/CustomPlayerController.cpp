// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomPlayerController.h"
#include "CustomComponents/CustomAnimInstance.h"
#include "VS_FSMCharacter.h"
#include "InputActionValue.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Equips/EquipComponent.h"

class UShootingComponent;

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
	if (CustomAnimInstance->bIsAiming) return;
	
	bToggleJogPressedExecuted = true;
	
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
	{
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->bIsJogging;	// serve all'ABP per capire quale transizione prendere, se walk o jog ( Deve prendere il valore vecchio)
	}
	ResolveGait();
}

void ACustomPlayerController::OnJogReleased()
{
	if (!bToggleJogPressedExecuted || CustomAnimInstance->bIsAiming) return;
	
	bToggleJogPressedExecuted = false;
	
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
	{
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->bIsJogging; // cosi ABP tiene traccia per i cambi
	}
	ResolveGait();
}

void ACustomPlayerController::OnEquipPressed()
{
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,  FString::Printf(TEXT("Equip Premuto")));
	/*	
	CustomAnimInstance->bFlare = true;
	if (UEquipComponent* EquipComponent = GetPawn() ? GetPawn()->FindComponentByClass<UEquipComponent>() : nullptr)
	{
		EquipComponent->Equip();
	}
	*/
}

void ACustomPlayerController::OnEquipReleased()
{
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,  FString::Printf(TEXT("Equip Rilasciato")));
		
	/*
	CustomAnimInstance->bFlare = false;
	if (UEquipComponent* EquipComponent = GetPawn() ? GetPawn()->FindComponentByClass<UEquipComponent>() : nullptr)
	{
		EquipComponent->UnEquip();
	}
	*/
}

void ACustomPlayerController::OnToggleWeapon()
{
	if (!ShootingComponent->bIsShootingComponentActive) return;
	
	ShootingComponent->SetWeaponEquip();
}

void ACustomPlayerController::OnChangeStance()		// Testing purpose, LEVA il binding al Tasto e collegalo ad un Delegate interno che gestisce i gait.
{
	EStanceMode NewStanceMode = PlayerCharacter->GetStanceMode() == EStanceMode::Alert ? EStanceMode::Normal : EStanceMode::Alert;
	PlayerCharacter->SetStanceMode(NewStanceMode);
	
	ResolveGait();
	
	GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::Emerald,
	FString::Printf(TEXT("Stance Mode Cambiata : %s"), *UEnum::GetValueAsString(NewStanceMode)));
}

void ACustomPlayerController::OnAimPressed()
{
	if (ShootingComponent) ShootingComponent->SetAiming(true);
}

void ACustomPlayerController::OnAimReleased()
{
	if (ShootingComponent) ShootingComponent->SetAiming(false);
}

void ACustomPlayerController::OnAimChangedUI(bool bAiming)
{
	if (!CrosshairWidget && CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UUserWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget) CrosshairWidget->AddToViewport();
	}
	if (CrosshairWidget)
		CrosshairWidget->SetVisibility(bAiming ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
	
	StickMagnitude = MovementVector.Size();
	ResolveGait();
	
	const FVector2D MaxInput = MovementVector.GetSafeNormal();
	PlayerCharacter->DoMove(MaxInput.X, MaxInput.Y);
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
	StickMagnitude = 0.f;
	ResolveGait();
}

void ACustomPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	PlayerCharacter = Cast<AVS_FSMCharacter>(InPawn);
	if (PlayerCharacter && PlayerCharacter->GetMesh())
		CustomAnimInstance = Cast<UCustomAnimInstance>(PlayerCharacter->GetMesh()->GetAnimInstance());
	else
		UE_LOG(LogTemp, Warning, TEXT("CustomAnimInstance cast skipped: move it inside the function where you actually need it, don't cast upfront"));
	
	ShootingComponent = PlayerCharacter->FindComponentByClass<UShootingSystem>();
	if (!ShootingComponent)
		UE_LOG(LogTemp, Warning, TEXT("OnPossess: ShootingComponent non trovato sul BP del character"));
	
	if (ShootingComponent)
		ShootingComponent->OnAimChanged.AddDynamic(this, &ACustomPlayerController::OnAimChangedUI);
}

void ACustomPlayerController::OnUnPossess()
{
	if (ShootingComponent)
		ShootingComponent->OnAimChanged.RemoveAll(this);
	PlayerCharacter = nullptr;
	CustomAnimInstance = nullptr;
	ShootingComponent = nullptr;
	Super::OnUnPossess();
}

void ACustomPlayerController::SetupInputActions(UEnhancedInputComponent* EIC)
{
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoJump);
	EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoCrouch);
	EIC->BindAction(ToggleJogAction, ETriggerEvent::Started, this, &ACustomPlayerController::OnJogPressed);
	EIC->BindAction(ToggleJogAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnJogReleased);
	EIC->BindAction(ToggleEquipAction, ETriggerEvent::Started, this, &ACustomPlayerController::OnEquipPressed);
	EIC->BindAction(ToggleEquipAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnEquipReleased);
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Move);
	EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnMoveCompleted);
	EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ACustomPlayerController::OnMoveCompleted);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Look);
	EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Look);
	EIC->BindAction(ToggleWeapon, ETriggerEvent::Started, this, &ACustomPlayerController::OnToggleWeapon);
	EIC->BindAction(ChangeStance, ETriggerEvent::Started, this, &ACustomPlayerController::OnChangeStance);
	EIC->BindAction(AimAction, ETriggerEvent::Started, this, &ACustomPlayerController::OnAimPressed);
	EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ACustomPlayerController::OnAimReleased);
}

void ACustomPlayerController::ResolveGait()
{
	if (!PlayerCharacter || !CustomAnimInstance) return;
	
	bool bWantJog = false;
	bool bWantRun = false;
	
	if (!CustomAnimInstance->bIsAiming)
	{
		if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
		{
			bWantJog = bIsUsingController && StickMagnitude >= JogStickThreshold;
			bWantRun = bToggleJogPressedExecuted;
		}
		else  // Normal per ora, aggiungi else if se aumentano i gait.
		{
			bWantJog = bToggleJogPressedExecuted;
			bWantRun = false;
		}
	}
	
	CustomAnimInstance->bIsJogging = bWantJog;
	CustomAnimInstance->bIsRunning = bWantRun;
}

FJumpSignature* ACustomPlayerController::GetJumpDelegate()
{
	return &JumpDelegate;
}

FCrouchSignature* ACustomPlayerController::GetCrouchDelegate()
{
	return &CrouchDelegate;
}