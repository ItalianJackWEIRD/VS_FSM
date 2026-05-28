// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CustomComponents/LocomotionTypes.h"
#include "StateManagerComponent.h"
#include "VS_FSMCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;


/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AVS_FSMCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:
	/** StanceMode -> manage poses and contextual actions (eg: Alert, Normal, NormalRelaxed) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stance")
	EStanceMode StanceMode = EStanceMode::Normal;

public:

	/** Constructor */
	AVS_FSMCharacter();	
	
protected:
	virtual void BeginPlay() override;

public:
	// Getter and Setter for StanceMode
	UFUNCTION(BlueprintPure, Category = "Stance")
	EStanceMode GetStanceMode() const { return StanceMode;}
	UFUNCTION(BlueprintCallable, Category = "Stance")
	void SetStanceMode(EStanceMode NewStance);
	
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStateManagerComponent* StateManager;
	
	/** Check the velocity and return if the character is moving **/
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual bool IsMoving() const;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

