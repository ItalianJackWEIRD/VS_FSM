// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/ShootingSystem/CornerMarker.h"
#include "CustomComponents/ShootingSystem/CornerRegistrySubsystem.h"
#include "Components/ArrowComponent.h"


ACornerMarker::ACornerMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("WallDir"));
	DirectionArrow->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void ACornerMarker::BeginPlay()
{
	Super::BeginPlay();
	if (UCornerRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCornerRegistrySubsystem>())
		Registry->Register(this);
	
}

void ACornerMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
		if (UCornerRegistrySubsystem* Registry = World->GetSubsystem<UCornerRegistrySubsystem>())
			Registry->Unregister(this);
	Super::EndPlay(EndPlayReason);
}

