// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "VSCameraLeanModifier.generated.h"

class UVSCameraComponent;
/**
 * 
 */
UCLASS()
class VSCAMERA_API UVSCameraLeanModifier : public UCameraModifier
{
	GENERATED_BODY()
	
public:
	TWeakObjectPtr<UVSCameraComponent> OwnerComp; // in CameraComponent gets populated. 
	
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;
	
};
