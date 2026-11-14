#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PipeActor.generated.h"

class UArrowComponent;
class UBoxComponent;
class UStaticMeshComponent;

/** Si allunga o accorcia con HEIGHT mai da Editor, altrimenti si deformerebbero anche i box. */
UCLASS()
class ULS_API APipeActor : public AActor
{
	GENERATED_BODY()
	
public:
	APipeActor();
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
	float GetBaseZ() const { return GetActorLocation().Z; }
	
	float GetTopZ() const { return GetBaseZ() + Height; }
	
	/** Serve per ottenere la direzione verso il muro, serve per fare dei calcoli sul commit e detection. */
	FVector GetClimbFacing() const { return FRotator(0.0f, GetActorRotation().Yaw, 0.0f).Vector(); }
	
	/** Dove deve stare la capsula per essere in presa alla base: posizione del centro e rotazione. */
	FTransform GetGripTransform(float GripDistance, float CapsuleHalfHeight) const;
	
	UBoxComponent* GetBottomEntry() const { return BottomEntry; }
	UBoxComponent* GetTopEntry() const { return TopEntry; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pipe")
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pipe")
	TObjectPtr<UArrowComponent> Arrow;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pipe")
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pipe")
	TObjectPtr<UBoxComponent> BottomEntry;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pipe")
	TObjectPtr<UBoxComponent> TopEntry;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pipe", meta=(ClampMin="200"))
	float Height = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pipe", meta=(ClampMin="1"))
	float Diameter = 12.f;
	
};
