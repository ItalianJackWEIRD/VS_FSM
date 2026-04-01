// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerBaseState.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "Kismet/GameplayStatics.h"

void UPlayerBaseState::OnEnterState(AActor* OwnerRef)
{
	//Super::OnEnterState(OwnerRef);	--> it's empty for now
	
	//Save player ref for later
	if (!PlayerRef)
		PlayerRef = Cast<AVS_FSMCharacter>(OwnerRef);
	
	//Save PlayerController*
	if (!PlayerController)
		PlayerController = Cast<ICustomPlayerControllerInterface>(UGameplayStatics::GetPlayerController(this, 0));
	
	//Bind jump-delegate
	PlayerController->GetJumpDelegate()->AddUObject(this, &UPlayerBaseState::OnJump);
}

void UPlayerBaseState::OnExitState()
{
	//Super::OnExitState();		--> empty
	
	PlayerController->GetJumpDelegate()->RemoveAll(this);
}

void UPlayerBaseState::OnJump()
{
	
}
