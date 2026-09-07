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
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->MovementGait == EMovementGait::Jog;	// serve all'ABP per capire quale transizione prendere, se walk o jog ( Deve prendere il valore vecchio)
	}
}

void ACustomPlayerController::OnJogReleased()
{
	if (!bToggleJogPressedExecuted || CustomAnimInstance->bIsAiming) return;
	
	bToggleJogPressedExecuted = false;
	
	if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert)
	{
		CustomAnimInstance->bTransitionRunInJog = CustomAnimInstance->MovementGait == EMovementGait::Jog; // cosi ABP tiene traccia per i cambi
	}
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

void ACustomPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	ResolveGait(DeltaTime);
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
	// bMoveInputActive = !MovementVector.IsNearlyZero();	--> ora lo scrive ResolveGait();
	
	if (!PlayerCharacter) return;	
	
	StickMagnitude = MovementVector.Size();
	
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
	// bMoveInputActive = false;	--> lo scrive ResolveGait();
	StickMagnitude = 0.f;
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

/*
 * Funzione che si occupa di gestire input dallo stick del controller 
 * durante Alert Gait (doppia funzione per stick). La scrittura diventa un evento. Sempre.
 */
void ACustomPlayerController::ResolveGait(float DeltaTime)
{
	if (!PlayerCharacter || !CustomAnimInstance) return;

	switch (StickSection)
	{
		case EStickInputSection::Inner:
			if (StickMagnitude > DeadzoneThreshold)
			{
				ReloadStickTimers();
				bMoveInputActive = true;
				StickSection = EStickInputSection::Middle;
				CustomAnimInstance->MovementGait = EMovementGait::Walk;
				break;
			}
		
			CenterTimer = FMath::Max(CenterTimer - DeltaTime, 0.0f);
			if (CenterTimer <= 0.0f)
			{
				bMoveInputActive = false;
			}
			break;
		
		case EStickInputSection::Middle:
			if (StickMagnitude > JogStickThreshold)
			{
				ReloadStickTimers();
				StickSection = EStickInputSection::Outer;
				CustomAnimInstance->MovementGait = 
					PlayerCharacter->GetStanceMode() == EStanceMode::Alert ? EMovementGait::Jog : EMovementGait::Walk;
				break;
			}
			if (StickMagnitude <= DeadzoneThreshold)
			{
				ReloadStickTimers();
				StickSection = EStickInputSection::Inner;
				CaptureMovStopSnapshot();
				break;
			}
		
			WalkTimer = FMath::Max(WalkTimer - DeltaTime, 0.0f);
			if (WalkTimer <= 0.0f)
			{
				CustomAnimInstance->MovementGait = EMovementGait::Walk;
			}
			break;
		
		case EStickInputSection::Outer:
		{
			if (StickMagnitude < JogStickThreshold)
			{
				ReloadStickTimers();
				StickSection = EStickInputSection::Middle;
				break;
			}
				
			EMovementGait Target = PlayerCharacter->GetStanceMode() == EStanceMode::Alert ? EMovementGait::Jog : EMovementGait::Walk;
			if (CustomAnimInstance->MovementGait != Target)
				CustomAnimInstance->MovementGait = Target;
			break;
		}
	}
	
	if (bToggleJogPressedExecuted && StickSection != EStickInputSection::Inner)
	{
		if (PlayerCharacter->GetStanceMode() == EStanceMode::Alert && StickSection == EStickInputSection::Outer && CustomAnimInstance->OrientationDirection == EOrientationDirection::Forward)
		{
			CustomAnimInstance->MovementGait = EMovementGait::Run;
		}
		else
		{
			CustomAnimInstance->MovementGait = EMovementGait::Jog;
		}
	}
	if (CustomAnimInstance->bIsAiming)
	{
		EMovementGait Target = EMovementGait::Walk;
		if (CustomAnimInstance->MovementGait != Target) CustomAnimInstance->MovementGait = Target;
	}
}

void ACustomPlayerController::ReloadStickTimers()
{
	CenterTimer = CenterCommitTime;
	WalkTimer = WalkCommitTime;
}

void ACustomPlayerController::CaptureMovStopSnapshot()
{
	if (!PlayerCharacter || !CustomAnimInstance) return;
	
	CustomAnimInstance->bMovStopJogging =
		PlayerCharacter->GetVelocity().Size2D() > CustomAnimInstance->MovStopJogSpeedThreshold;
	CustomAnimInstance->bMovStopCrouched = CustomAnimInstance->bIsCrouched;
}

FJumpSignature* ACustomPlayerController::GetJumpDelegate()
{
	return &JumpDelegate;
}

FCrouchSignature* ACustomPlayerController::GetCrouchDelegate()
{
	return &CrouchDelegate;
}