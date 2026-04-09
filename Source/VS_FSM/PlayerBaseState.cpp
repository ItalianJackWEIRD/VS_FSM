// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerBaseState.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void UPlayerBaseState::OnEnterState(AActor* OwnerRef)
{	
	//Save player ref for later
	if (!PlayerRef)
		PlayerRef = Cast<AVS_FSMCharacter>(OwnerRef);
	
	//Save CMC
	if (!CharacterMovementComponent && PlayerRef)
		CharacterMovementComponent = PlayerRef->GetCharacterMovement();
	
	//Save PlayerController*
	if (!PlayerController)
		PlayerController = Cast<ICustomPlayerControllerInterface>(UGameplayStatics::GetPlayerController(this, 0));
	
	//Bind jump-delegate
	PlayerController->GetJumpDelegate()->AddUObject(this, &UPlayerBaseState::OnJump);
	
	//Import State Data
	if (IsValid(StateData) && CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = StateData->MovementSpeed;
		CharacterMovementComponent->MaxAcceleration = StateData->MaxAcceleration;
		CharacterMovementComponent->BrakingDecelerationWalking = StateData->BrakingDeceleration;
		CharacterMovementComponent->BrakingFrictionFactor = StateData->BrakingFrictionFactor;
		CharacterMovementComponent->BrakingFriction = StateData->BrakingFriction;
		CharacterMovementComponent->bUseSeparateBrakingFriction = StateData->bUseSeparateBrakingFriction;
	}
}

void UPlayerBaseState::OnExitState()
{
	//Super::OnExitState();		--> empty
	
	PlayerController->GetJumpDelegate()->RemoveAll(this);
}

void UPlayerBaseState::OnJump()
{
	
}
