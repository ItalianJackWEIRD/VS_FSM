// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CornerRegistrySubsystem.generated.h"

class ACornerMarker;

UCLASS()
class VS_FSM_API UCornerRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void Register(ACornerMarker* Marker)   { Corners.AddUnique(Marker); }
	void Unregister(ACornerMarker* Marker) { Corners.Remove(Marker); }

	ACornerMarker* FindNearest(const FVector& Location, float MaxRadius) const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACornerMarker>> Corners;
};
