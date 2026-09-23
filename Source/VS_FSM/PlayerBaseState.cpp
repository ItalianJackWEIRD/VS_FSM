// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerBaseState.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "CustomComponents/VSCharacterMovementComponent.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

bool UPlayerBaseState::IsMoving() const
{
	return PlayerRef && PlayerRef->GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER;
}

void UPlayerBaseState::OnEnterState(AActor* OwnerRef)
{	
	//Save player ref for later
	if (!PlayerRef)
		PlayerRef = Cast<ACharacter>(OwnerRef);
	
	if (!StateManager && PlayerRef)
		StateManager = PlayerRef->FindComponentByClass<UStateManagerComponent>();
	
	//Save CMC
	if (!CharacterMovementComponent && PlayerRef)
		CharacterMovementComponent = PlayerRef->GetCharacterMovement();
	
	//Save PlayerController*
	if (!PlayerController)
		PlayerController = Cast<ICustomPlayerControllerInterface>(UGameplayStatics::GetPlayerController(this, 0));
	
	if (!LocoComp && PlayerRef)
		LocoComp = PlayerRef->FindComponentByClass<ULocomotionStateComponent>();
	
	//Save CameraComponent
	if (!CameraRef && PlayerRef)
		CameraRef = Cast<UVSCameraComponent>(PlayerRef->FindComponentByClass<UVSCameraComponent>());
	
	//Bind Delegates
	SetupDelegates();
	
	ApplyMovementParameters();
}

void UPlayerBaseState::OnExitState()
{
	//Super::OnExitState();		--> empty
	
	ResetDelegates();
}

void UPlayerBaseState::SetupDelegates()
{
	PlayerController->GetJumpDelegate()->AddUObject(this, &UPlayerBaseState::OnJump);
	PlayerController->GetCrouchDelegate()->AddUObject(this, &UPlayerBaseState::OnCrouch);
	if (LocoComp) LocoComp->StanceChangedDelegate.AddUObject(this, &UPlayerBaseState::ApplyMovementParameters);
}

void UPlayerBaseState::ResetDelegates()
{
	PlayerController->GetJumpDelegate()->RemoveAll(this);
	PlayerController->GetCrouchDelegate()->RemoveAll(this);
	if (LocoComp) LocoComp->StanceChangedDelegate.RemoveAll(this);
}

void UPlayerBaseState::ApplyMovementParameters()
{
	StateData = ResolveStateData();
	
	if (!IsValid(StateData) || !CharacterMovementComponent || !LocoComp)  return;
	
	//Import State Data and Refresh in Custom Anim Instance
	CharacterMovementComponent->MaxWalkSpeed = StateData->MovementSpeed;
	CharacterMovementComponent->MaxAcceleration = StateData->MaxAcceleration;
	CharacterMovementComponent->BrakingDecelerationWalking = StateData->BrakingDeceleration;
	CharacterMovementComponent->BrakingFrictionFactor = StateData->BrakingFrictionFactor;
	CharacterMovementComponent->BrakingFriction = StateData->BrakingFriction;
	CharacterMovementComponent->bUseSeparateBrakingFriction = StateData->bUseSeparateBrakingFriction;
	CharacterMovementComponent->RotationRate = FRotator(0.f, StateData->RotationRate, 0.f);
		
	LocoComp->LeanStateIndex = StateData->LeanStateIndex;
	LocoComp->TargetPlayRate = StateData->PlayRate;
	LocoComp->RefreshMovementCache();
	
	
	if (UVSCharacterMovementComponent* VSMove = Cast<UVSCharacterMovementComponent>(CharacterMovementComponent))
	{
		VSMove->LateralScale  = StateData->LateralSpeedScale;
		VSMove->BackwardScale = StateData->BackwardSpeedScale;
	}
}

const ULocomotionDataAsset* UPlayerBaseState::ResolveStateData() const
{
	const EStanceMode Stance = LocoComp ? LocoComp->StanceMode : EStanceMode::Normal;
	if (const TObjectPtr<ULocomotionDataAsset>* Found = State_StanceData.Find(Stance))
		if (*Found) return *Found;

	const TObjectPtr<ULocomotionDataAsset>* Fallback = State_StanceData.Find(EStanceMode::Normal);
	return Fallback ? *Fallback : nullptr;
}


void UPlayerBaseState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
}


// To override
void UPlayerBaseState::OnJump()
{
}

void UPlayerBaseState::OnCrouch()
{
}


