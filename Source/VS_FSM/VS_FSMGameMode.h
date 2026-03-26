// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VS_FSMGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AVS_FSMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AVS_FSMGameMode();
};



