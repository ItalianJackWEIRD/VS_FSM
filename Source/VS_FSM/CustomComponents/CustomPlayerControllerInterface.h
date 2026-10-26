// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CustomPlayerControllerInterface.generated.h"



UINTERFACE(MinimalAPI)
class UCustomPlayerControllerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VS_FSM_API ICustomPlayerControllerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool IsMovementInputZero() const = 0;
};
