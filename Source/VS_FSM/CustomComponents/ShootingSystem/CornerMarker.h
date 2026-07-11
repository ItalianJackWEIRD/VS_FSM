// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornerMarker.generated.h"

class UArrowComponent;

/**
 * Punto di corner piazzato a mano nel livello.
 * La freccia (ActorForward) è la WallDir: corre LUNGO il muro ed ESCE dal corner verso lo spazio aperto.
 */
UCLASS()
class VS_FSM_API ACornerMarker : public AActor
{
	GENERATED_BODY()
	
public:	
	ACornerMarker();

	FVector GetWallDir() const { return GetActorForwardVector().GetSafeNormal2D(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category="Corner")
	TObjectPtr<UArrowComponent> DirectionArrow = nullptr;

};
