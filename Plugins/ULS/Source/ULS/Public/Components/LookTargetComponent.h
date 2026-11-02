// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "LookTargetComponent.generated.h"

class UIkContextComponent;

/**
 * Something worth looking at. Put it on any actor: when a character with an IkContext
 * enters the sphere, this registers with it; when it leaves, it unregisters.
 * The sphere radius is the range. Attach it to a socket (e.g. an NPC head) or use LookOffset.
 * No tick, no scans: only overlap events.
 */
UCLASS(ClassGroup=(Locomotion), meta=(BlueprintSpawnableComponent))
class ULS_API ULookTargetComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	ULookTargetComponent();

	/** Look point, relative to this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookTarget")
	FVector LookOffset = FVector::ZeroVector;

	/** Higher priority wins over closer targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookTarget")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookTarget")
	bool bLookTargetEnabled = true;
	
	/** Looked at even in Alert stance, where the other targets are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookTarget")
	bool bIgnoreAlert = false;

	FVector GetLookLocation() const { return GetComponentTransform().TransformPosition(LookOffset); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void RegisterWith(AActor* Other);
	void UnregisterFrom(AActor* Other);

	/** Who we are registered with, to unregister on EndPlay. */
	TArray<TWeakObjectPtr<UIkContextComponent>> Watchers;
};