// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/ShootingSystem.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "ShootingSystem//WeaponDataAsset.h"
#include "ShootingSystem//WeaponBase.h"
#include "VS_FSMCharacter.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "CustomComponents/CustomAnimInstance.h" 

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<bool> CVarShowCQBDebug(
	TEXT("MyGame.ShowCQBDebug"),
	false,   // default: disattivato
	TEXT("Mostra le sfere di debug per i CQB probes (0 = off, 1 = on)"),
	ECVF_Default
);
#endif

// Sets default values for this component's properties
UShootingSystem::UShootingSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShootingSystem::SelectWeapon(UWeaponDataAsset* NewWeapon)
{
	CurrentWeaponData = NewWeapon;
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(true);
}

void UShootingSystem::Arm()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!CurrentWeaponData || !CurrentWeaponData->WeaponActorClass || !Mesh) return;
	
	if (EquippedWeapon) Disarm();
	
	BreathingComponent->SwitchOn();
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(false);
	
	const FName Socket = CurrentWeaponData->AttachSocketName;
	const FTransform SocketTransform = Mesh->GetSocketTransform(Socket);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	EquippedWeapon = GetWorld()->SpawnActor<AWeaponBase>(CurrentWeaponData->WeaponActorClass, SocketTransform, SpawnParams);
	
	if (!EquippedWeapon) return;
	
	bHasWeapon = true;
	
	EquippedWeapon->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	
	if (CustomAnimInstance)
	{
		CustomAnimInstance->WeaponGrip = CurrentWeaponData->Grip;
		CustomAnimInstance->OverlayReadyStand = CurrentWeaponData->OverlayAnims.ReadyStand;
		CustomAnimInstance->OverlayReadyCrouch = CurrentWeaponData->OverlayAnims.ReadyCrouch;
		CustomAnimInstance->OverlayStand2H = CurrentWeaponData->Overlay2H.Stand;
		CustomAnimInstance->OverlayCrouch2H = CurrentWeaponData->Overlay2H.Crouch;
		CustomAnimInstance->OverlayJog2H = CurrentWeaponData->Overlay2H.Jog;
		CustomAnimInstance->TransitionJogWalk2H = CurrentWeaponData->Overlay2H.JogToWalk;
		CustomAnimInstance->TransitionWalkJog2H = CurrentWeaponData->Overlay2H.WalkToJog;
	}
	if (CurrentWeaponData->Grip == EWeaponGrip::Mixed)
		StartProximityScan();
	
	bIsTransitioning = false;
}

void UShootingSystem::Disarm()
{
	bHasWeapon 	= false;
	if (EquippedWeapon) EquippedWeapon->Destroy();
	EquippedWeapon 	= nullptr;
	bIsTransitioning = false;
	if (HolsterMeshComp) HolsterMeshComp->SetVisibility(true);
	CustomAnimInstance->bUpperBodyOn = false;
	BreathingComponent->SwitchOff();
	StopProximityScan(); // Always, Fallback...
}


void UShootingSystem::SetWeaponEquip()
{
	// WHOLE LOGIC: Set Abp parameters (Which is put false in OnEventExitBP) , On/Off Breathing System in Arm() Disarm(), set Alpha to 1 (bIsTransitioning).
	if (!CurrentWeaponData) return;
	
	CustomAnimInstance->bUpperBodyOn = true;
	
	bIsTransitioning = true;
	CustomAnimInstance->bShouldEquipWeapon = true;
	
	if (!bHasWeapon)
	{
		if (CustomAnimInstance->bIsCrouched)
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->EquipAnimationCrouch;
		else
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->EquipAnimationStand;
	}
	else
	{
		if (CustomAnimInstance->bIsCrouched)
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->UnEquipAnimationCrouch;
		else
			CustomAnimInstance->EquipUnEquipAnim = CurrentWeaponData->UnEquipAnimationStand;
	}
}

void UShootingSystem::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		CustomAnimInstance = Cast<UCustomAnimInstance>(Mesh->GetAnimInstance());
		BreathingComponent = Cast<UBreathingComponent>(GetOwner()->FindComponentByClass<UBreathingComponent>());
	}

	if (!CustomAnimInstance)
		UE_LOG(LogTemp, Warning, TEXT("ShootingSystem: CustomAnimInstance nulla a BeginPlay"));
	
	// Default di test finché non arriva l'Inventory col D-pad. Mettiamo tutto in SetupNewWeapon()
	if (!CurrentWeaponData)
		CurrentWeaponData = DefaultWeaponData;
	
	SetupNewWeapon();
	
	SetupHolsterMesh();
}


void UShootingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!CustomAnimInstance) return;
	
	if (bIsTransitioning)
	{
		if (CurrentWeaponData->Grip == EWeaponGrip::OneHand)
			CustomAnimInstance->Weapon1hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon1hAlpha, 1, DeltaTime, WeaponInterpSpeed);
		else if (CurrentWeaponData->Grip == EWeaponGrip::TwoHand)
			CustomAnimInstance->Weapon2hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon2hAlpha, 1, DeltaTime, WeaponInterpSpeed);
		else if (CurrentWeaponData->Grip == EWeaponGrip::Mixed)
		{
			const float Target1H = bInTightSpace ? 0.f : 1;
			const float Target2H = bInTightSpace ? 1 : 0.f;
			CustomAnimInstance->Weapon1hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon1hAlpha, Target1H, DeltaTime, WeaponInterpSpeed);
			CustomAnimInstance->Weapon2hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon2hAlpha, Target2H, DeltaTime, WeaponInterpSpeed);
		}
		
		return;
	}
	
	const float Target = ComputeTargetAlpha();
	
	if (CurrentWeaponData->Grip == EWeaponGrip::OneHand)
		CustomAnimInstance->Weapon1hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon1hAlpha, Target, DeltaTime, WeaponInterpSpeed);
	else if (CurrentWeaponData->Grip == EWeaponGrip::TwoHand)
		CustomAnimInstance->Weapon2hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon2hAlpha, Target, DeltaTime, WeaponInterpSpeed);
	else if (CurrentWeaponData->Grip == EWeaponGrip::Mixed)
	{
		const float Target1H = bInTightSpace ? 0.f : Target;
		const float Target2H = bInTightSpace ? Target : 0.f;
		CustomAnimInstance->Weapon1hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon1hAlpha, Target1H, DeltaTime, WeaponInterpSpeed);
		CustomAnimInstance->Weapon2hAlpha = FMath::FInterpTo(CustomAnimInstance->Weapon2hAlpha, Target2H, DeltaTime, WeaponInterpSpeed);
	}
	
	float Alpha;	// GRIP GUN
	if (CurrentWeaponData->Grip == EWeaponGrip::TwoHand)
		Alpha = CustomAnimInstance->Weapon2hAlpha;
	else if (CurrentWeaponData->Grip == EWeaponGrip::Mixed)
		Alpha = FMath::Max(CustomAnimInstance->Weapon1hAlpha, CustomAnimInstance->Weapon2hAlpha);
	else
		Alpha = CustomAnimInstance->Weapon1hAlpha;
	CustomAnimInstance->GripAlpha = bHasWeapon ? (1.f - Alpha) : 0.f; // cause weaponAlpha is already interpolated
	
	GEngine->AddOnScreenDebugMessage(77, 0.f, FColor::Yellow,
	FString::Printf(TEXT("Upper: %d 1hA: %f 2hA: %f TightSpace: %s"), CustomAnimInstance->bUpperBodyOn, CustomAnimInstance->Weapon1hAlpha, CustomAnimInstance->Weapon2hAlpha, bInTightSpace ? TEXT("TRUE") : TEXT("FALSE")));
}

USkeletalMeshComponent* UShootingSystem::GetOwnerMesh() const
{
	if (const ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		return Owner->GetMesh();
	}
	return nullptr;
}

float UShootingSystem::ComputeTargetAlpha() const
{
	if (!bHasWeapon) return 0.f;
	if (bRunAlphaOverride) return 0.f;
	
	const float StanceBase = (GetStanceMode() == EStanceMode::Alert) ? 1.f : 0.f;
	return FMath::Max(StanceBase, BreathingAlpha);		// if u are in normal, breathing takes over, otherwise in alert its always 1.
}

EStanceMode UShootingSystem::GetStanceMode() const
{
	if (const AVS_FSMCharacter* Char = Cast<AVS_FSMCharacter>(GetOwner()))
		return Char->GetStanceMode();
	return EStanceMode::Normal; //Fallback
}

void UShootingSystem::SetupHolsterMesh()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh || !CurrentWeaponData || !CurrentWeaponData->HolsterMesh) return;

	const FName Socket = CurrentWeaponData->HolsterSocketName;
	if (!Mesh->DoesSocketExist(Socket))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShootingSystem: holster socket '%s' inesistente sullo skeleton in-game"), *Socket.ToString());
		return;
	}

	if (!HolsterMeshComp)
	{
		HolsterMeshComp = NewObject<UStaticMeshComponent>(GetOwner());
		HolsterMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HolsterMeshComp->RegisterComponent();
	}

	HolsterMeshComp->SetStaticMesh(CurrentWeaponData->HolsterMesh);
	HolsterMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	HolsterMeshComp->SetVisibility(!bHasWeapon); // visibile solo quando l'arma NON è in mano
}

void UShootingSystem::SetupNewWeapon()
{
	switch (CurrentWeaponData->Grip)
	{
		case EWeaponGrip::OneHand:
			CustomAnimInstance->bUpper1H = true;
			CustomAnimInstance->bUpper2H = false;
			break;
		
		case EWeaponGrip::TwoHand:
			CustomAnimInstance->bUpper1H = false;
			CustomAnimInstance->bUpper2H = true;
			break;
		
		case EWeaponGrip::Mixed:
			CustomAnimInstance->bUpper1H = true;
			CustomAnimInstance->bUpper2H = true;
			break;
		
		case EWeaponGrip::Melee:
			CustomAnimInstance->bUpper1H = false;
			CustomAnimInstance->bUpper2H = false;
			break;
	}
}

/* MIXED SYSTEM */

void UShootingSystem::StartProximityScan()
{
	UWorld* World = GetWorld();
	if (!World) return;
	World->GetTimerManager().SetTimer(ProximityTimerHandle, this, &UShootingSystem::TickProximityScan, ProximityInterval, true);
	TickProximityScan();
}

void UShootingSystem::StopProximityScan()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(ProximityTimerHandle);
	
	bInTightSpace = false;
}

void UShootingSystem::TickProximityScan()
{
	const bool bNow = ProbeForCover();
	if (bNow == bInTightSpace) return;
	
	bInTightSpace = bNow;
}

bool UShootingSystem::ProbeForCover() const
{
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World || CoverObjectTypes.Num() == 0) return false;
	
	FCollisionObjectQueryParams ObjParams(CoverObjectTypes);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CQBProximity), false);
	QueryParams.AddIgnoredActor(Owner);
	
	const bool bHit = World->OverlapAnyTestByObjectType(
		Owner->GetActorLocation(),
		FQuat::Identity,
		ObjParams,
		FCollisionShape::MakeSphere(CQBProbesRadius),
		QueryParams);
	
#if ENABLE_DRAW_DEBUG
	if (CVarShowCQBDebug.GetValueOnGameThread())
	{
		DrawDebugSphere(World, Owner->GetActorLocation(), CQBProbesRadius, 16,
			bHit ? FColor::Red : FColor::Green, false, ProximityInterval);
	}
#endif

	return bHit;
}
