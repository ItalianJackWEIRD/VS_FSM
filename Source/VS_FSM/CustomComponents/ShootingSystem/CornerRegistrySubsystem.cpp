
#include "CustomComponents/ShootingSystem/CornerRegistrySubsystem.h"
#include "CustomComponents/ShootingSystem/CornerMarker.h"

ACornerMarker* UCornerRegistrySubsystem::FindNearest(const FVector& Location, float MaxRadius) const
{
	ACornerMarker* Best = nullptr;
	float BestDistSq = FMath::Square(MaxRadius);

	for (ACornerMarker* Marker : Corners)
	{
		if (!IsValid(Marker)) continue;
		const float DistSq = FVector::DistSquared2D(Location, Marker->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Marker;
		}
	}
	return Best;
}
