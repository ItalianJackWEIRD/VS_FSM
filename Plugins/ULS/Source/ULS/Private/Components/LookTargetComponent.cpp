// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LookTargetComponent.h"

#include "Components/IkContextComponent.h"
#include "GameFramework/Actor.h"

ULookTargetComponent::ULookTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Range sphere: queries only, overlaps pawns, ignores everything else.
	SphereRadius = 500.f;
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	ShapeColor = FColor::Cyan;
}

void ULookTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &ULookTargetComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &ULookTargetComponent::HandleEndOverlap);

	// Actors already inside when the game starts never get a begin overlap.
	TArray<AActor*> Inside;
	GetOverlappingActors(Inside);
	for (AActor* Actor : Inside)
		RegisterWith(Actor);
}

void ULookTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<UIkContextComponent>& Watcher : Watchers)
		if (UIkContextComponent* Ik = Watcher.Get())
			Ik->UnregisterLookTarget(this);
	Watchers.Reset();

	Super::EndPlay(EndPlayReason);
}

void ULookTargetComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	RegisterWith(OtherActor);
}

void ULookTargetComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	// Another component of the same actor may still be inside.
	if (OtherActor && !IsOverlappingActor(OtherActor))
		UnregisterFrom(OtherActor);
}

void ULookTargetComponent::RegisterWith(AActor* Other)
{
	if (!Other || Other == GetOwner()) return;
	if (UIkContextComponent* Ik = Other->FindComponentByClass<UIkContextComponent>())
	{
		Ik->RegisterLookTarget(this);
		Watchers.AddUnique(Ik);
	}
}

void ULookTargetComponent::UnregisterFrom(AActor* Other)
{
	if (!Other) return;
	if (UIkContextComponent* Ik = Other->FindComponentByClass<UIkContextComponent>())
	{
		Ik->UnregisterLookTarget(this);
		Watchers.Remove(Ik);
	}
}