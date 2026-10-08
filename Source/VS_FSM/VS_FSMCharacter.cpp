// Copyright Epic Games, Inc. All Rights Reserved.

#include "VS_FSMCharacter.h"

#include "VSCameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LocomotionStateComponent.h"
#include "Components/VSCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "States/PlayerBaseState.h"

/**
* ACharacter crea il suo movement component nel proprio costruttore, prima che il tuo corpo giri. 
* Quando arrivi a GetCharacterMovement() il componente esiste già, quindi non puoi sostituirlo dall'interno.
* 
* SetDefaultSubobjectClass interviene un attimo prima: dice a Super "quando crei quel subobject, usa questa classe invece di quella 
* di default". Ecco perché sta nella lista di inizializzazione e non nel corpo.
* 
ACharacter::CharacterMovementComponentName è il nome con cui ACharacter registra quel subobject — devi indicarlo perché è così che il sistema lo identifica.
 * @param ObjectInitializer 
 */
AVS_FSMCharacter::AVS_FSMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVSCharacterMovementComponent>(
		  ACharacter::CharacterMovementComponentName))
{
	// State Manager
	StateManager = CreateDefaultSubobject<UStateManagerComponent>(TEXT("StateManager"));
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	// GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	
	LocoComp = CreateDefaultSubobject<ULocomotionStateComponent>(TEXT("LocomotionState"));
	PrimaryActorTick.bCanEverTick = true;
}

void AVS_FSMCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (LocoComp) LocoComp->SetLocomotionLayer(LocoComp->LocomotionType);
	StateManager->InitStateManager();
	if (LocoComp) LocoComp->StanceMode = StanceMode;
}

void AVS_FSMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LocoComp)
		if (UVSCameraComponent* Cam = FindComponentByClass<UVSCameraComponent>())
			Cam->SetLeanAngle(LocoComp->LeanAngle);
}

void AVS_FSMCharacter::SetStanceMode(EStanceMode NewStance)
{
	if (StanceMode == NewStance) return;
	StanceMode = NewStance;
	if (LocoComp)
	{
		LocoComp->StanceMode = NewStance;
		LocoComp->StanceChangedDelegate.Broadcast();
	}
	StanceChangedDelegate.Broadcast();
}

void AVS_FSMCharacter::ToggleStance()
{
	SetStanceMode(LocoComp->StanceMode == EStanceMode::Alert ? EStanceMode::Normal : EStanceMode::Alert);
}

void AVS_FSMCharacter::SwapLoco()
{
	if (LocoComp) SetLocomotionBackend(LocoComp->IsFSMBackend() ? ELocomotionBackend::MM : ELocomotionBackend::FSM);
}

/** 
 * Questa fx differisce da quella chiamata dentro LocoComp perchè quando
 *  swappi a RunTime il BACKEND bisogna refreshare anche i valori del CMC. Se lo si cambia
 *  ad inizio livello gli OnEnterState gestiscono questo.
 */
void AVS_FSMCharacter::SetLocomotionBackend(ELocomotionBackend NewType)
{
	if (!LocoComp || LocoComp->LocomotionType == NewType) return;
	
	LocoComp->SetLocomotionLayer(NewType);
	// Riapplichiamo i valori del CMC dal DA
	if (UPlayerBaseState* State = Cast<UPlayerBaseState>(StateManager->CurrentState))
		State->ApplyMovementParameters();		
}

void AVS_FSMCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AVS_FSMCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AVS_FSMCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AVS_FSMCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

bool AVS_FSMCharacter::IsMoving() const
{
	return GetCharacterMovement()->Velocity.SizeSquared() > KINDA_SMALL_NUMBER;
}
