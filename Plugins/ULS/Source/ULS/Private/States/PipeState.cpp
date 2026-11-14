// Fill out your copyright notice in the Description page of Project Settings.


#include "States/PipeState.h"
#include "Data/PipeDataAsset.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPipeState::OnEnterState(AActor* OwnerRef)
{
	Super::OnEnterState(OwnerRef);
	Pipe = LocoComp->PendingPipe;
	PipeDataAsset = Cast<UPipeDataAsset>(StateData);
	
	LocoComp->LocomotionMode = ELocomotionMode::Pipe;
	bExitCrouched = LocoComp->bIsCrouched;
	LocoComp->bIsCrouched = false;
	LocoComp->ResetFSMTransientState();
	
	if (AController* PC = PlayerRef->GetController()) PC->SetIgnoreMoveInput(true);
	CharacterMovementComponent->StopMovementImmediately();
	CharacterMovementComponent->SetMovementMode(MOVE_Flying);
	// RICORDA DA RotationRate = 0;
}

void UPipeState::OnExitState()
{
	if (AController* PC = PlayerRef->GetController()) PC->SetIgnoreMoveInput(false);
	if (CharacterMovementComponent->MovementMode == MOVE_Flying)	// se siamo in FALLING rimanici
		CharacterMovementComponent->SetMovementMode(MOVE_Walking);
	Pipe = nullptr;
	Super::OnExitState();
}

void UPipeState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	if (!Pipe || !PipeDataAsset)
	{
		StateManager->SwitchStateByKey("Idle");
		return;
	}
	UpdateAnimationParameters(DeltaTime);
}

void UPipeState::OnCrouch()
{
	// Staccati dal tubo, con la pushForce. 
}
