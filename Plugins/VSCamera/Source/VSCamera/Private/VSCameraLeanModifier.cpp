// Fill out your copyright notice in the Description page of Project Settings.


#include "VSCameraLeanModifier.h"

#include "VSCameraComponent.h"

bool UVSCameraLeanModifier::ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV)
{
	if (UVSCameraComponent* Comp = OwnerComp.Get() )
	{
		InOutPOV.Rotation.Roll = Comp->GetCurrentLeanRoll();
	}
	return false; // false = lascia che altri modifier (shake futuro) modifichino ancora la POV
}
