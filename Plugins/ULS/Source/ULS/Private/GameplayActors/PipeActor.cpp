#include "GameplayActors/PipeActor.h"	// il path della tua cartella

#include "ULS.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** Box invisibile che risponde solo allo sweep del tubo: non blocca player, camera né altri trace. */
	void SetupEntryBox(UBoxComponent* Box, const FVector& RelativeLocation)
	{
		Box->InitBoxExtent(FVector(15.f, 20.f, 75.f));	// mezze misure: 30 profondo, 40 largo, 150 alto
		Box->SetRelativeLocation(RelativeLocation);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(ECC_PipeEntry, ECR_Block);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCanEverAffectNavigation(false);
	}
}

APipeActor::APipeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);		// appeso, la capsula si sovrappone al tubo
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);	// la camera non scatta sul tubo
	Mesh->SetCanEverAffectNavigation(false);

	// Davanti al tubo, dal lato del player (X negativa), da terra a 150.
	BottomEntry = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomEntry"));
	BottomEntry->SetupAttachment(Root);
	SetupEntryBox(BottomEntry, FVector(-15.f, 0.f, 75.f));

	// Oltre il muro, sul pavimento di sopra. La Z la sistema OnConstruction.
	TopEntry = CreateDefaultSubobject<UBoxComponent>(TEXT("TopEntry"));
	TopEntry->SetupAttachment(Root);
	SetupEntryBox(TopEntry, FVector(40.f, 0.f, Height + 75.f));
}

void APipeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Mesh adattata a Height e Diameter, centrata sull'asse e con la base a terra,
	// qualunque siano le sue misure e il suo pivot.
	if (const UStaticMesh* StaticMesh = Mesh->GetStaticMesh())
	{
		const FBox Bounds = StaticMesh->GetBoundingBox();
		const FVector Size = Bounds.GetSize();
		if (Size.X > KINDA_SMALL_NUMBER && Size.Z > KINDA_SMALL_NUMBER)
		{
			const float ScaleXY = Diameter / Size.X;
			const float ScaleZ = Height / Size.Z;
			const FVector Center = Bounds.GetCenter();

			Mesh->SetRelativeScale3D(FVector(ScaleXY, ScaleXY, ScaleZ));
			Mesh->SetRelativeLocation(FVector(-Center.X * ScaleXY, -Center.Y * ScaleXY, -Bounds.Min.Z * ScaleZ));
		}
	}

	// Il box in cima segue Height. X e Y restano quelli impostati nel Blueprint.
	FVector TopLocation = TopEntry->GetRelativeLocation();
	TopLocation.Z = Height + TopEntry->GetUnscaledBoxExtent().Z;
	TopEntry->SetRelativeLocation(TopLocation);
}

FTransform APipeActor::GetGripTransform(float GripDistance, float CapsuleHalfHeight) const
{
	const FVector Location = GetActorLocation()
		- GetClimbFacing() * GripDistance			// indietro rispetto al muro
		+ FVector::UpVector * CapsuleHalfHeight;	// centro della capsula, coi piedi a terra

	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), Location);
}