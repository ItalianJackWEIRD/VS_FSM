// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomPlayerController.h"
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

void ACustomPlayerController::SetupInputActions(UEnhancedInputComponent* EIC)
{
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoJump);
	EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ACustomPlayerController::DoCrouch);
}

FJumpSignature* ACustomPlayerController::GetJumpDelegate()
{
	return &JumpDelegate;
}

FCrouchSignature* ACustomPlayerController::GetCrouchDelegate()
{
	return &CrouchDelegate;
}
