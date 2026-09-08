// Copyright Epic Games, Inc. All Rights Reserved.

#include "VS_FSMCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CustomComponents/VSCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"

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
}

void AVS_FSMCharacter::BeginPlay()
{
	Super::BeginPlay();
	StateManager->InitStateManager();
}

void AVS_FSMCharacter::SetStanceMode(EStanceMode NewStance)
{
	if (StanceMode == NewStance) return;
	StanceMode = NewStance;
	StanceChangedDelegate.Broadcast();
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
