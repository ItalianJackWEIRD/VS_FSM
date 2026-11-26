// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/TraversalComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/CustomAnimInstance.h"
#include "AnimationWarpingLibrary.h"
#include "Chooser.h"
#include "ChooserFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "IObjectChooser.h"
#include "MotionWarpingComponent.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchResult.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "ULS.h"

namespace
{
	TAutoConsoleVariable<int32> CVarTraversalDrawDebugLevel(
		TEXT("uls.traversal.DrawDebugLevel"), 0,
		TEXT("Debug del traversal: 0 spento, 1 bordi, spazi ed esito, 2 anche tutte le tracce."));

	TAutoConsoleVariable<float> CVarTraversalDrawDebugDuration(
		TEXT("uls.traversal.DrawDebugDuration"), 2.f,
		TEXT("Secondi di vita del debug di una prova singola. In AutoCheck dura un frame."));

	TAutoConsoleVariable<bool> CVarTraversalAutoCheck(
		TEXT("uls.traversal.AutoCheck"), false,
		TEXT("Prova il traversal a ogni frame senza eseguirlo: serve a tarare rilevamento e scelta."));
	
	TAutoConsoleVariable<bool> CVarDropDownDebugPersist(
		TEXT("uls.dropdown.DebugPersist"), false,
		TEXT("Il debug del drop-down resta per uls.traversal.DrawDebugDuration secondi invece che un frame: serve a fotografarlo."));
	
	/** I bordi si cercano 1 cm dentro la faccia. */
	constexpr float FaceInset = 1.f;
	/** Le tracce verso un bordo finiscono un po' oltre il punto di partenza: reggono anche se il punto sta sullo spigolo. */
	constexpr float EdgeTraceOvershoot = 10.f;
	/** Una faccia conta come verticale se la Z della sua normale non supera questo valore. */
	constexpr float MaxFaceNormalZ = 0.5f;
	/** Distanza tra capsula e geometria nelle sweep di spazio, come in GASP. */
	constexpr float RoomPadding = 2.f;

	/** Fin dove cercare, partendo dal punto sulla faccia. Oltre, la traccia parte dentro la geometria e non vale. */
	constexpr float TopSearchHeight = 300.f;	// la chooser arriva a 275
	constexpr float SideSearchDistance = 100.f;
	constexpr float BackSearchDepth = 150.f;	// più profondo di così è un blocco su cui salire: Mantle

	FCollisionQueryParams MakeQueryParams(const AActor* Owner)
	{
		// Collisione semplice: è quella contro cui si muove anche la capsula.
		return FCollisionQueryParams(SCENE_QUERY_STAT(Traversal), false, Owner);
	}
}

FString FTraversalCheckResult::ToString() const
{
	return FString::Printf(
		TEXT("%s | alt %.0f  prof %.0f  dietro %.0f | front %d  back %d  floor %d | %s  da %.2fs  x%.2f"),
		*StaticEnum<ETraversalActionType>()->GetNameStringByValue(static_cast<int64>(ActionType)),
		ObstacleHeight, ObstacleDepth, BackLedgeHeight,
		static_cast<int32>(bHasFrontLedge), static_cast<int32>(bHasBackLedge), static_cast<int32>(bHasBackFloor),
		*GetNameSafe(ChosenMontage), StartTime, PlayRate);
}

UTraversalComponent::UTraversalComponent()
{
	// Il tick serve al fix della capsula che clippa col terreno. 
	PrimaryComponentTick.bCanEverTick = true;
}

void UTraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		UE_LOG(LogULS, Error, TEXT("UTraversalComponent: l'owner deve essere un ACharacter."));
		return;
	}
	CharacterMovement = Character->GetCharacterMovement();
	Capsule = Character->GetCapsuleComponent();
	MotionWarping = Character->FindComponentByClass<UMotionWarpingComponent>();
	
	if (const ACharacter* CDO = Character->GetClass()->GetDefaultObject<ACharacter>())
		DefaultCapsuleRadius = CDO->GetCapsuleComponent()->GetScaledCapsuleRadius();

	if (!MotionWarping)
		UE_LOG(LogULS, Warning, TEXT("UTraversalComponent: manca UMotionWarpingComponent sul character."));
	if (!TraversalChooser)
		UE_LOG(LogULS, Warning, TEXT("UTraversalComponent: TraversalChooser non assegnata nel Blueprint del character."));
}

void UTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bDoingTraversal) UpdateGroundSnap(DeltaTime);
	
#if !UE_BUILD_SHIPPING
	if (DebugExitFrames > 0)
	{
		LogExitFrame(*FString::Printf(TEXT("+%d"), 16 - DebugExitFrames));
		--DebugExitFrames;
	}
#endif
#if !UE_BUILD_SHIPPING
	// Solo debug: prova a ogni frame senza eseguire, per tarare rilevamento e scelta. Mai durante un traversal.
	if (CVarTraversalAutoCheck.GetValueOnGameThread() && !bDoingTraversal)
	{
		bAutoChecking = true;
		FTraversalCheckResult Result;
		TryTraversal(Result);
		bAutoChecking = false;
	}
#endif
}

bool UTraversalComponent::TryTraversal(FTraversalCheckResult& OutResult)
{
	const bool bOk = RunCheck(OutResult);
	if (bOk) LastCheckResult = OutResult;	// lo esegue lo stato Traversal appena entra

#if !UE_BUILD_SHIPPING
	const FString Summary = bOk ? OutResult.ToString() : FString::Printf(TEXT("niente (%s)"), *LastFailReason);

	if (ShouldDrawDebug(1) && GEngine)
		GEngine->AddOnScreenDebugMessage(50, DebugLifeTime(), bOk ? FColor::Green : FColor::Silver, TEXT("Traversal: ") + Summary);

	// Nel log solo quando cambia l'esito, non a ogni frame.
	const FString LogKey = bOk
		? FString::Printf(TEXT("%d %s"), static_cast<int32>(OutResult.ActionType), *GetNameSafe(OutResult.ChosenMontage))
		: LastFailReason;
	if (!bAutoChecking || LogKey != LastLogKey)
	{
		UE_LOG(LogULS, Log, TEXT("Traversal: %s"), *Summary);
		LastLogKey = LogKey;
	}
#endif

	return bOk;
}

bool UTraversalComponent::RunCheck(FTraversalCheckResult& Result)
{
	Result = FTraversalCheckResult();
	LastFailReason.Reset();
	if (!Character || !CharacterMovement || !Capsule) return Fail(TEXT("riferimenti del character mancanti"));

	const UWorld* World = GetWorld();
	const FVector Up = FVector::UpVector;
	const FTraversalCheckInputs In = GetTraversalCheckInputs();

	// Capsula IN PIEDI anche da accucciati: il traversal parte sempre in piedi, quindi spazi e
	// altezze si misurano così. Da accucciati il risultato è lo stesso che in piedi.
	const float Radius = DefaultCapsuleRadius;
	const float StandHalfHeight = Character->GetDefaultHalfHeight();
	const FVector CapsuleLocation = Capsule->GetComponentLocation();
	const float FeetZ = CapsuleLocation.Z - Capsule->GetScaledCapsuleHalfHeight();
	const FVector StandCenter(CapsuleLocation.X, CapsuleLocation.Y, FeetZ + StandHalfHeight);

	// 1. Ostacolo davanti.
	FHitResult ObstacleHit;
	const FVector TraceEnd = StandCenter + In.Direction * In.Distance + In.EndOffset;
	const bool bObstacle = SweepCapsule(StandCenter, TraceEnd, In.Radius, In.HalfHeight, ObstacleChannel, ObstacleHit);
	if (ShouldDrawDebug(2))
	{
		DrawDebugCapsule(World, StandCenter, In.HalfHeight, In.Radius, FQuat::Identity, FColor::Silver, false, DebugLifeTime());
		DrawDebugCapsule(World, bObstacle ? FVector(ObstacleHit.Location) : TraceEnd, In.HalfHeight, In.Radius,
			FQuat::Identity, bObstacle ? FColor::Green : FColor::Red, false, DebugLifeTime());
	}
	if (!bObstacle) return Fail(TEXT("nessun ostacolo"));
	if (!IsTraversable(ObstacleHit)) return Fail(TEXT("ostacolo escluso: fisica attiva, personaggio o tag"));
	Result.HitComponent = ObstacleHit.GetComponent();

	// 2. Bordi, calcolati dalla geometria colpita.
	if (!ComputeLedges(ObstacleHit, In.Direction, Result)) return false;	// il motivo lo scrive ComputeLedges
	DrawLedges(Result);

	// 3. Spazio fino al bordo anteriore: la capsula in piedi deve arrivare sopra il bordo.
	const float Clearance = Radius + RoomPadding;
	const FVector RoomFront = Result.FrontLedgeLocation + Result.FrontLedgeNormal * Clearance + Up * (StandHalfHeight + RoomPadding);
	FHitResult RoomHit;
	const bool bFrontBlocked = SweepCapsule(StandCenter, RoomFront, Radius, StandHalfHeight, RoomChannel, RoomHit);
	DrawRoom(RoomFront, Radius, StandHalfHeight, bFrontBlocked);
	if (bFrontBlocked)
	{
		Result.bHasFrontLedge = false;
		return Fail(TEXT("niente spazio fino al bordo"));
	}
	Result.ObstacleHeight = FMath::Abs(FeetZ - Result.FrontLedgeLocation.Z);

	// 4. Spazio sopra l'ostacolo, e pavimento dietro.
	if (Result.bHasBackLedge)
	{
		const FVector RoomBack = Result.BackLedgeLocation + Result.BackLedgeNormal * Clearance + Up * (StandHalfHeight + RoomPadding);
		FHitResult AcrossHit;
		const bool bAcrossBlocked = SweepCapsule(RoomFront, RoomBack, Radius, StandHalfHeight, RoomChannel, AcrossHit);
		DrawRoom(RoomBack, Radius, StandHalfHeight, bAcrossBlocked);

		if (bAcrossBlocked)
		{
			// Qualcosa sopra l'ostacolo: non si scavalca, al massimo ci si sale fin lì.
			Result.ObstacleDepth = (AcrossHit.ImpactPoint - Result.FrontLedgeLocation).Size2D();
			Result.bHasBackLedge = false;
		}
		else
		{
			Result.ObstacleDepth = (Result.FrontLedgeLocation - Result.BackLedgeLocation).Size2D();

			// Scende dietro il bordo posteriore, fino a 50 cm sotto il pavimento di partenza.
			const FVector FloorEnd = Result.BackLedgeLocation + Result.BackLedgeNormal * Clearance
				- Up * (Result.ObstacleHeight - StandHalfHeight + 50.f);
			FHitResult FloorHit;
			if (SweepCapsule(RoomBack, FloorEnd, Radius, StandHalfHeight, RoomChannel, FloorHit))
			{
				Result.bHasBackFloor = true;
				Result.BackFloorLocation = FloorHit.ImpactPoint;
				Result.BackLedgeHeight = FMath::Abs(FloorHit.ImpactPoint.Z - Result.BackLedgeLocation.Z);
				if (ShouldDrawDebug(1))
					DrawDebugSphere(World, FloorHit.ImpactPoint, 8.f, 12, FColor::Yellow, false, DebugLifeTime());
			}
			else if (ShouldDrawDebug(2))
			{
				DrawDebugCapsule(World, FloorEnd, StandHalfHeight, Radius, FQuat::Identity, FColor::Orange, false, DebugLifeTime());
			}
		}
	}
	else
	{
		// Retro non trovato (blocco profondo, terreno): quanto spazio c'è sopra per salirci.
		const FVector AcrossEnd = RoomFront - Result.FrontLedgeNormal * (MantleRoomDepth + Clearance);
		FHitResult AcrossHit;
		const bool bAcrossBlocked = SweepCapsule(RoomFront, AcrossEnd, Radius, StandHalfHeight, RoomChannel, AcrossHit);
		DrawRoom(bAcrossBlocked ? FVector(AcrossHit.Location) : AcrossEnd, Radius, StandHalfHeight, bAcrossBlocked);
		Result.ObstacleDepth = bAcrossBlocked
			? (AcrossHit.ImpactPoint - Result.FrontLedgeLocation).Size2D()
			: MantleRoomDepth;
	}

	// 5. Tipo d'azione, montaggio, tempo d'inizio e play rate.
	return ChooseAction(Result);
}

FTraversalCheckInputs UTraversalComponent::GetTraversalCheckInputs() const
{
	FTraversalCheckInputs In;
	In.Direction = Character->GetActorForwardVector();
	In.Radius = TraceRadius;

	const EMovementMode Mode = CharacterMovement->MovementMode;
	if (Mode == MOVE_Falling || Mode == MOVE_Flying)
	{
		In.Distance = AirTraceDistance;
		In.EndOffset = FVector(0.f, 0.f, AirTraceEndOffsetZ);
		In.HalfHeight = AirTraceHalfHeight;
	}
	else	// Walking, NavWalking, Swimming, Custom, None
	{
		const double ForwardSpeed = Character->GetActorRotation().UnrotateVector(CharacterMovement->Velocity).X;
		In.Distance = static_cast<float>(FMath::GetMappedRangeValueClamped(GroundSpeedRange, GroundTraceDistanceRange, ForwardSpeed));
		In.HalfHeight = GroundTraceHalfHeight;
	}
	return In;
}

bool UTraversalComponent::IsTraversable(const FHitResult& Hit) const
{
	const UPrimitiveComponent* Comp = Hit.GetComponent();
	if (!Comp || Comp->IsSimulatingPhysics()) return false;	// un oggetto che si muove non si scavalca
	if (Comp->ComponentHasTag(NoTraversalTag)) return false;

	const AActor* Actor = Hit.GetActor();
	if (Cast<APawn>(Actor)) return false;						// nemmeno un altro personaggio
	return !(Actor && Actor->ActorHasTag(NoTraversalTag));
}

/*
 * Bordi calcolati dalla geometria colpita, non da attori disegnati a mano come in GASP.
 * L'idea viene da GASP-Refactored di Anaylan (licenza MIT): tracce fatte solo contro la
 * componente colpita, lungo il piano della faccia, partendo da fuori e tornando verso il punto.
 * Il primo urto è il bordo della componente in quella direzione.
 */
bool UTraversalComponent::ComputeLedges(const FHitResult& Hit, const FVector& TraceDirection, FTraversalCheckResult& Result) const
{
	UPrimitiveComponent* Comp = Hit.GetComponent();
	if (!Comp) return Fail(TEXT("componente colpita non valida"));

	const FVector Up = FVector::UpVector;

	// La sweep a capsula può prendere lo spigolo e dare una normale storta: la faccia vera
	// la dà una linea orizzontale 5 cm più in basso, solo contro questa componente.
	const FVector Dir = TraceDirection.GetSafeNormal2D();
	const FVector Probe = Hit.ImpactPoint - Up * 5.f;
	FHitResult FaceHit;
	if (!Comp->LineTraceComponent(FaceHit, Probe - Dir * 50.f, Probe + Dir * 50.f, MakeQueryParams(GetOwner()))
		|| FMath::Abs(FaceHit.ImpactNormal.Z) > MaxFaceNormalZ)
	{
		return Fail(TEXT("faccia anteriore non verticale"));
	}
	const FVector FrontNormal = FaceHit.ImpactNormal.GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(Up, FrontNormal);	// lungo la faccia, in orizzontale

	// Cima: dall'alto verso il basso, 1 cm dentro la faccia. Il primo urto è il bordo anteriore.
	FHitResult TopHit;
	if (!FindEdgeAlong(Comp, FaceHit.ImpactPoint - FrontNormal * FaceInset, Up, TopSearchHeight, TopHit)
		|| TopHit.ImpactNormal.Z < MinLedgeTopNormalZ)
	{
		return Fail(TEXT("nessuna cima raggiungibile su cui appoggiarsi"));
	}
	FVector Ledge = TopHit.ImpactPoint;

	// Larghezza, 5 cm sotto la cima: il bordo resta ad almeno MinLedgeWidth/2 da entrambi gli spigoli.
	const FVector WidthProbe = Ledge - Up * 5.f;
	const float HalfWidth = MinLedgeWidth * 0.5f;
	FHitResult EdgeHit;
	const float RightDist = FindEdgeAlong(Comp, WidthProbe, Side, SideSearchDistance, EdgeHit)
		? FVector::Dist(EdgeHit.ImpactPoint, WidthProbe) : SideSearchDistance;
	const float LeftDist = FindEdgeAlong(Comp, WidthProbe, -Side, SideSearchDistance, EdgeHit)
		? FVector::Dist(EdgeHit.ImpactPoint, WidthProbe) : SideSearchDistance;
	if (RightDist + LeftDist < MinLedgeWidth) return Fail(TEXT("troppo stretto"));

	if (RightDist < HalfWidth || LeftDist < HalfWidth)
	{
		// Troppo vicino a uno spigolo: sposta il bordo verso l'interno e rimisura la cima lì.
		const FVector Shift = RightDist < HalfWidth ? -Side * (HalfWidth - RightDist) : Side * (HalfWidth - LeftDist);
		if (!FindEdgeAlong(Comp, WidthProbe + Shift, Up, TopSearchHeight, TopHit) || TopHit.ImpactNormal.Z < MinLedgeTopNormalZ)
			return Fail(TEXT("nessuna cima raggiungibile su cui appoggiarsi"));
		Ledge = TopHit.ImpactPoint;
	}

	Result.bHasFrontLedge = true;
	Result.FrontLedgeLocation = Ledge;
	Result.FrontLedgeNormal = FrontNormal;

	// Retro: da dietro l'ostacolo verso il bordo, 2 cm sotto la cima. Il primo urto è la faccia posteriore.
	// Niente retro entro BackSearchDepth: è un blocco su cui salire, e lo gestisce il controllo dello spazio sopra.
	FHitResult BackFaceHit;
	if (!FindEdgeAlong(Comp, Ledge - Up * 2.f, -FrontNormal, BackSearchDepth, BackFaceHit)
		|| FMath::Abs(BackFaceHit.ImpactNormal.Z) > MaxFaceNormalZ)
	{
		return true;
	}
	const FVector BackNormal = BackFaceHit.ImpactNormal.GetSafeNormal2D();

	FHitResult BackTopHit;
	if (!FindEdgeAlong(Comp, BackFaceHit.ImpactPoint - BackNormal * FaceInset, Up, TopSearchHeight, BackTopHit))
		return true;

	Result.bHasBackLedge = true;
	Result.BackLedgeLocation = BackTopHit.ImpactPoint;
	Result.BackLedgeNormal = BackNormal;
	return true;
}

bool UTraversalComponent::FindEdgeAlong(UPrimitiveComponent* Comp, const FVector& InsidePoint, const FVector& Dir, float Reach, FHitResult& OutHit) const
{
	// Parte lontano lungo Dir e torna verso il punto. Se parte già dentro la geometria, non vale.
	const FVector Start = InsidePoint + Dir * Reach;
	const FVector End = InsidePoint - Dir * EdgeTraceOvershoot;
	const bool bHit = Comp->LineTraceComponent(OutHit, Start, End, MakeQueryParams(GetOwner()))
		&& !OutHit.bStartPenetrating && OutHit.Time > KINDA_SMALL_NUMBER;

	if (ShouldDrawDebug(2))
		DrawDebugLine(GetWorld(), Start, bHit ? FVector(OutHit.ImpactPoint) : End, bHit ? FColor::Cyan : FColor::Orange, false, DebugLifeTime());
	return bHit;
}

bool UTraversalComponent::SweepCapsule(const FVector& Start, const FVector& End, float Radius, float HalfHeight, ECollisionChannel Channel, FHitResult& OutHit) const
{
	// True anche se la capsula parte compenetrata: per noi vuol dire "niente spazio".
	return GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel,
		FCollisionShape::MakeCapsule(Radius, HalfHeight), MakeQueryParams(GetOwner()));
}

bool UTraversalComponent::ChooseAction(FTraversalCheckResult& Result) const
{
	// Input della chooser: geometria, velocità 2D e movement mode del CMC.
	FTraversalChooserInputs Inputs;
	Inputs.bHasFrontLedge = Result.bHasFrontLedge;
	Inputs.bHasBackLedge = Result.bHasBackLedge;
	Inputs.bHasBackFloor = Result.bHasBackFloor;
	Inputs.ObstacleHeight = Result.ObstacleHeight;
	Inputs.ObstacleDepth = Result.ObstacleDepth;
	Inputs.BackLedgeHeight = Result.BackLedgeHeight;
	Inputs.MovementMode = CharacterMovement->MovementMode;
	Inputs.Speed = CharacterMovement->Velocity.Size2D();
	Inputs.bIsDropDown = false;

	UAnimMontage* Montage = nullptr;
	if (!EvaluateChooser(Inputs, Result.FrontLedgeLocation, Result.FrontLedgeNormal,
		Result.ActionType, Montage, Result.StartTime, Result.PlayRate))
	{
		return false;
	}
	Result.ChosenMontage = Montage;
	return true;
}

bool UTraversalComponent::EvaluateChooser(FTraversalChooserInputs Inputs, const FVector& LedgeLocation, const FVector& LedgeNormal,
	ETraversalActionType& OutType, UAnimMontage*& OutMontage, float& OutStartTime, float& OutPlayRate) const
{
	if (!TraversalChooser) return Fail(TEXT("TraversalChooser non assegnata"));
	UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	if (!HostAnim) return Fail(TEXT("anim instance dell'host mancante"));

	// 1. Valutazione multipla: escono tutti i montaggi validi, e la riga scelta nella tabella
	//    principale scrive l'ActionType. L'ordine è quello dei Parameters: 0 Input, 1 Output.
	FTraversalChooserOutputs Outputs;
	FChooserEvaluationContext Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddStructParam(Inputs);
	Context.AddStructParam(Outputs);
	TArray<UObject*> Montages = UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
		Context, UChooserFunctionLibrary::MakeEvaluateChooser(TraversalChooser), UAnimMontage::StaticClass());

	OutType = Outputs.ActionType;
	if (OutType == ETraversalActionType::None || Montages.IsEmpty())
		return Fail(TEXT("nessuna riga della chooser per questo ostacolo"));

	// 2. Il bordo all'host: lo leggono i canali di PSD_Traversal durante il Motion Match.
	HostAnim->SetLedgeTransform(FTransform(FRotationMatrix::MakeFromZ(LedgeNormal).ToQuat(), LedgeLocation));

	// 3. Motion Match sui montaggi candidati: sceglie il montaggio e il frame d'ingresso
	//    in base alla distanza dal bordo e alla posa attuale.
	FPoseSearchBlueprintResult Match;
	UPoseSearchLibrary::MotionMatch(HostAnim, Montages, PoseHistoryName,
		FPoseSearchContinuingProperties(), FPoseSearchFutureProperties(), Match);

	// Il Motion Match restituisce l'asset come const: per suonarlo serve non-const.
	OutMontage = const_cast<UAnimMontage*>(Cast<const UAnimMontage>(Match.SelectedAnim));
	if (!OutMontage) return Fail(TEXT("il Motion Match non ha scelto un montaggio"));
	OutStartTime = Match.SelectedTime;
	OutPlayRate = Match.WantedPlayRate;
	return true;
}

/* ---> ESECUZIONE (D5-D6)
**/

bool UTraversalComponent::PerformTraversal(const FTraversalCheckResult& Result)
{
	UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!HostAnim || !Mesh || !Result.ChosenMontage || !CharacterMovement || !Capsule) return false;
	if (bDoingTraversal) AbortTraversal();

	ActiveResult = Result;
	UpdateWarpTargets();
	
	// Fine dell'ultima finestra di warp: da lì i piedi vanno verso il pavimento d'arrivo.
	LastWarpWindowEnd = 0.f;
	TArray<FMotionWarpingWindowData> Windows;
	UMotionWarpingUtilities::GetMotionWarpingWindowsFromAnimation(Result.ChosenMontage, Windows);
	for (const FMotionWarpingWindowData& Window : Windows)
		LastWarpWindowEnd = FMath::Max(LastWarpWindowEnd, Window.EndTime);
	
#if !UE_BUILD_SHIPPING
	// Diagnosi del climb a 2,5 m: se il montaggio parte dopo la finestra FrontLedge, il warp non parte.
	{
		TArray<FMotionWarpingWindowData> WindowsLog;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(Result.ChosenMontage, FrontLedgeWarpName, WindowsLog);
		if (WindowsLog.IsEmpty())
		{
			UE_LOG(LogULS, Warning, TEXT("Traversal: %s non ha una finestra %s"), *GetNameSafe(Result.ChosenMontage), *FrontLedgeWarpName.ToString());
		}
		else
		{
			const FMotionWarpingWindowData& W = WindowsLog[0];
			const TCHAR* Verdict = Result.StartTime >= W.EndTime ? TEXT("  -> OLTRE la finestra: niente warp")
				: Result.StartTime > W.StartTime ? TEXT("  -> dentro la finestra")
				: TEXT("");
			UE_LOG(LogULS, Log, TEXT("Traversal: %s parte da %.2fs, finestra %s %.2f-%.2fs%s"),
				*GetNameSafe(Result.ChosenMontage), Result.StartTime, *FrontLedgeWarpName.ToString(), W.StartTime, W.EndTime, Verdict);
		}
	}
#endif	
	
	// Il montaggio parte dal frame e alla velocità scelti dal Motion Match: è per questo che l'ingresso è morbido.
	if (HostAnim->Montage_Play(Result.ChosenMontage, Result.PlayRate, EMontagePlayReturnType::MontageLength, Result.StartTime) <= 0.f)
		return false;
	
	++TraversalSerial;
	FOnMontageBlendingOutStarted BlendingOut;
	BlendingOut.BindUObject(this, &UTraversalComponent::OnMontageBlendingOut, TraversalSerial);
	HostAnim->Montage_SetBlendingOutDelegate(BlendingOut, Result.ChosenMontage);
	FOnMontageEnded Ended;
	Ended.BindUObject(this, &UTraversalComponent::OnMontageEnded, TraversalSerial);
	HostAnim->Montage_SetEndDelegate(Ended, Result.ChosenMontage);

	bDoingTraversal = true;

	// La capsula passa attraverso l'ostacolo: la muove il root motion del montaggio.
	if (Result.HitComponent) Capsule->IgnoreComponentWhenMoving(Result.HitComponent, true);
	SavedMeshWorldDynamicResponse = Mesh->GetCollisionResponseToChannel(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);

	// Flying: il root motion può muovere anche in verticale. Non fa scattare InAir, che guarda solo Falling.
	CharacterMovement->SetMovementMode(MOVE_Flying);
	return true;
}

void UTraversalComponent::AbortTraversal()
{
	if (!bDoingTraversal) return;

	// Chi chiama Abort (lo stato che esce) non vuole essere richiamato: prima si staccano i delegate, poi si ferma.
	UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	if (HostAnim && ActiveResult.ChosenMontage)
	{
		FOnMontageBlendingOutStarted NoBlendingOut;
		HostAnim->Montage_SetBlendingOutDelegate(NoBlendingOut, ActiveResult.ChosenMontage);
		FOnMontageEnded NoEnded;
		HostAnim->Montage_SetEndDelegate(NoEnded, ActiveResult.ChosenMontage);
		HostAnim->Montage_Stop(AbortBlendOutTime, ActiveResult.ChosenMontage);
	}
	FinishTraversal(true, false);
}

void UTraversalComponent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 Serial)
{
	// Callback di un traversal già chiuso: col tasto tenuto il successivo può essere già partito.
	if (Serial != TraversalSerial) return;
	
#if !UE_BUILD_SHIPPING
	if (const UCustomAnimInstance* HostAnim = GetHostAnimInstance())
		UE_LOG(LogULS, Log, TEXT("Traversal: %s sfuma a %.2fs%s"), *GetNameSafe(Montage),
			HostAnim->Montage_GetPosition(Montage), bInterrupted ? TEXT(", interrotto") : TEXT(""));
#endif
	
	// Completato o interrotto, si chiude all'inizio dello sfumato: il controllo torna mentre
	// il montaggio sfuma, e il MM sceglie la sua posa prima di diventare visibile.
	FinishTraversal(bInterrupted, true);
}


void UTraversalComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 Serial)
{
	if (Serial != TraversalSerial) return;
	FinishTraversal(bInterrupted, true);	// di solito l'ha già chiuso il blend out: allora non fa niente
}

void UTraversalComponent::FinishTraversal(bool bInterrupted, bool bNotify)
{
	if (!bDoingTraversal) return;
	bDoingTraversal = false;

#if !UE_BUILD_SHIPPING
	LogExitFrame(TEXT("prima"));
#endif
	
	if (Capsule && ActiveResult.HitComponent) Capsule->IgnoreComponentWhenMoving(ActiveResult.HitComponent, false);
	if (USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr)
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, SavedMeshWorldDynamicResponse);

	// Dopo un vault si cade, negli altri casi si è a terra. Se sotto non c'è niente, il CMC passa a Falling da solo.
	const bool bToFalling = ActiveResult.ActionType == ETraversalActionType::Vault;

	// Prima di Walking la capsula va dove la vuole il CMC: se ci entra dentro il pavimento, il CMC
	// la spinge fuori e in più corregge l'altezza, e nel primo frame sale quasi del doppio.
	float GroundZ = 0.f;
	if (!bToFalling && GetGroundedCapsuleZ(GroundZ) && FMath::Abs(GroundZ - Character->GetActorLocation().Z) <= GroundSnapMaxDistance)
	{
		FVector Location = Character->GetActorLocation();
		Location.Z = GroundZ;
		Character->SetActorLocation(Location);
	}

	if (CharacterMovement)
		CharacterMovement->SetMovementMode(bToFalling ? MOVE_Falling : MOVE_Walking);
	
#if !UE_BUILD_SHIPPING
	LogExitFrame(TEXT("dopo SetMovementMode"));
	DebugExitFrames = 15;
#endif
	
	if (bNotify) OnTraversalFinished.Broadcast(bInterrupted);
}

void UTraversalComponent::UpdateWarpTargets()
{
	if (!MotionWarping) return;
	const FTraversalCheckResult& R = ActiveResult;

	// FrontLedge, sempre: sul bordo anteriore, rivolto verso l'ostacolo.
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(FrontLedgeWarpName,
		R.FrontLedgeLocation + FVector(0.f, 0.f, 0.5f), FRotationMatrix::MakeFromX(-R.FrontLedgeNormal).Rotator());

	// BackLedge: solo hurdle e vault, che passano sopra l'ostacolo.
	if (R.ActionType == ETraversalActionType::Hurdle || R.ActionType == ETraversalActionType::Vault)
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(BackLedgeWarpName, R.BackLedgeLocation, FRotator::ZeroRotator);
	else
		MotionWarping->RemoveWarpTarget(BackLedgeWarpName);

	// BackFloor: solo hurdle. La curva Distance_From_Ledge dice a che distanza dal bordo anteriore il montaggio
	// arriva sul bordo posteriore e poi a terra: l'atterraggio sta oltre il bordo posteriore di quella differenza.
	if (R.ActionType == ETraversalActionType::Hurdle)
	{
		float DistToBackLedge = 0.f;
		float DistToBackFloor = 0.f;
		GetCurveAtWarpWindowEnd(BackLedgeWarpName, DistToBackLedge);
		GetCurveAtWarpWindowEnd(BackFloorWarpName, DistToBackFloor);

		FVector FloorTarget = R.BackLedgeLocation + R.BackLedgeNormal * FMath::Abs(DistToBackLedge - DistToBackFloor);
		FloorTarget.Z = R.BackFloorLocation.Z;
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(BackFloorWarpName, FloorTarget, FRotator::ZeroRotator);
	}
	else
	{
		MotionWarping->RemoveWarpTarget(BackFloorWarpName);
	}
}

bool UTraversalComponent::GetCurveAtWarpWindowEnd(FName WarpTargetName, float& OutValue) const
{
	TArray<FMotionWarpingWindowData> Windows;
	UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(ActiveResult.ChosenMontage, WarpTargetName, Windows);
	if (Windows.IsEmpty()) return false;

	UAnimationWarpingLibrary::GetCurveValueFromAnimation(ActiveResult.ChosenMontage, DistanceFromLedgeCurveName, Windows[0].EndTime, OutValue);
	return true;
}

UCustomAnimInstance* UTraversalComponent::GetHostAnimInstance() const
{
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	return Mesh ? Cast<UCustomAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
}

bool UTraversalComponent::Fail(const TCHAR* Reason) const
{
	LastFailReason = Reason;
	return false;
}

bool UTraversalComponent::GetGroundedCapsuleZ(float& OutZ) const
{
	if (!Character || !CharacterMovement || !Capsule) return false;

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Start = Character->GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, HalfHeight + GroundSnapMaxDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TraversalGroundSnap), false, Character);
	const FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, Capsule->GetCollisionObjectType(), Params, Response)) return false;
	if (!CharacterMovement->IsWalkable(Hit)) return false;

	// In Walking il CMC tiene la capsula tra MIN_FLOOR_DIST (1,9) e MAX_FLOOR_DIST (2,4) sopra il pavimento: si punta a metà.
	OutZ = Hit.ImpactPoint.Z + HalfHeight + 2.15f;
	return true;
}

void UTraversalComponent::UpdateGroundSnap(float DeltaTime)
{
	if (ActiveResult.ActionType == ETraversalActionType::Vault) return;	// il vault finisce in caduta

	const UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	const UAnimMontage* Montage = ActiveResult.ChosenMontage;
	if (!HostAnim || !Montage || !HostAnim->Montage_IsPlaying(Montage)) return;

	// Solo a salita finita: dopo l'ultima finestra di warp e con il root motion verticale fermo.
	if (HostAnim->Montage_GetPosition(Montage) < LastWarpWindowEnd) return;
	if (FMath::Abs(CharacterMovement->Velocity.Z) > GroundSnapMaxVerticalSpeed) return;

	float TargetZ = 0.f;
	if (!GetGroundedCapsuleZ(TargetZ)) return;

	const float Error = TargetZ - Character->GetActorLocation().Z;
	if (FMath::Abs(Error) > GroundSnapMaxDistance) return;

	Character->AddActorWorldOffset(FVector(0.f, 0.f, Error * FMath::Clamp(DeltaTime * GroundSnapSpeed, 0.f, 1.f)));
}


/* ---> DROP-DOWN
**/

FString FDropDownCheckResult::ToString() const
{
	return FString::Printf(TEXT("%s%s | dislivello %.0f  spigolo a %.0f | %s  da %.2fs  x%.2f"),
		*StaticEnum<EDropDownType>()->GetNameStringByValue(static_cast<int64>(Type)),
		bForced ? TEXT(" (forzato)") : TEXT(""),
		DropHeight, DistanceToEdge, *GetNameSafe(Montage), StartTime, PlayRate);
}

bool UTraversalComponent::TryDropDown(const FDropDownSettings& Settings, FDropDownCheckResult& OutResult)
{
	// Lo chiamano gli stati a ogni tick: il debug dura un frame, a meno di uls.dropdown.DebugPersist.
	TGuardValue<bool> PerFrameDebug(bAutoChecking, !CVarDropDownDebugPersist.GetValueOnGameThread());

	const bool bOk = RunDropCheck(Settings, OutResult);
	if (bOk && OutResult.Type == EDropDownType::Low) LastDropResult = OutResult;	// lo esegue la TraversalState

#if !UE_BUILD_SHIPPING
	const FString Summary = bOk ? OutResult.ToString() : FString::Printf(TEXT("niente (%s)"), *LastFailReason);
	if (ShouldDrawDebug(1) && GEngine)
		GEngine->AddOnScreenDebugMessage(51, DebugLifeTime(), bOk ? FColor::Green : FColor::Silver, TEXT("DropDown: ") + Summary);
	
	// Distanza dal centro della capsula, non dai piedi: serve a tarare la finestra d'innesco.
	if (ShouldDrawDebug(1) && GEngine && OutResult.LedgeComponent)
		GEngine->AddOnScreenDebugMessage(53, DebugLifeTime(), FColor::Cyan,
			FString::Printf(TEXT("DropDown spigolo a %.0f cm  dislivello %.0f cm"), OutResult.DistanceToEdge, OutResult.DropHeight));
	
	// Nel log solo quando cambia l'esito, non a ogni frame.
	const FString LogKey = bOk ? FString::Printf(TEXT("%d %d"), static_cast<int32>(OutResult.Type), OutResult.bForced ? 1 : 0) : LastFailReason;
	if (LogKey != LastDropLogKey)
	{
		UE_LOG(LogULS, Log, TEXT("DropDown: %s"), *Summary);
		LastDropLogKey = LogKey;
	}
#endif

	return bOk;
}

bool UTraversalComponent::UpdateDropDownCommit(const FDropDownSettings& Settings, float DeltaTime)
{
	// Un timer solo: se il frame scorso nessuno ha chiamato (stato senza controllo, traversal...), riparte da zero.
	if (GFrameCounter != LastDropCommitFrame + 1) DropCommitElapsed = 0.f;
	LastDropCommitFrame = GFrameCounter;

	FDropDownCheckResult Result;
	if (!TryDropDown(Settings, Result) || Result.Type != EDropDownType::Low)
	{
		DropCommitElapsed = 0.f;
		return false;
	}

	// Sotto la soglia minima si parte subito: dal bordo non si cade.
	if (Result.bForced)
	{
		DropCommitElapsed = 0.f;
		return true;
	}

	DropCommitElapsed += DeltaTime;

#if !UE_BUILD_SHIPPING
	if (ShouldDrawDebug(1) && GEngine)
		GEngine->AddOnScreenDebugMessage(52, 0.f, FColor::Yellow,
			FString::Printf(TEXT("DropDown commit: %.2f / %.2f s"), DropCommitElapsed, Settings.CommitTime));
#endif

	if (DropCommitElapsed < Settings.CommitTime) return false;
	DropCommitElapsed = 0.f;
	return true;
}

bool UTraversalComponent::RunDropCheck(const FDropDownSettings& S, FDropDownCheckResult& Result)
{
	Result = FDropDownCheckResult();
	LastFailReason.Reset();
	if (!Character || !CharacterMovement || !Capsule) return Fail(TEXT("riferimenti del character mancanti"));
	if (!TraversalChooser) return Fail(TEXT("TraversalChooser non assegnata"));
	if (bDoingTraversal || bDoingDropDown) return Fail(TEXT("traversal o drop-down gia' in corso"));

	const UWorld* World = GetWorld();
	const FVector Up = FVector::UpVector;
	const FCollisionQueryParams Params = MakeQueryParams(GetOwner());
	const FVector CapsuleLocation = Capsule->GetComponentLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float FeetZ = CapsuleLocation.Z - HalfHeight;

	// Direzione: quella del movimento, non della camera. Da fermi, l'ultima in cui ci si muoveva:
	// dentro la soglia minima il drop-down parte anche cosi'.
	const FVector Velocity2D(CharacterMovement->Velocity.X, CharacterMovement->Velocity.Y, 0.f);
	const bool bStill = Velocity2D.SizeSquared() < FMath::Square(S.StillSpeed);
	if (!bStill) LastDropMoveDir = Velocity2D.GetSafeNormal();
	if (LastDropMoveDir.IsNearlyZero()) return Fail(TEXT("fermo, nessuna direzione di marcia"));
	const FVector Dir = LastDropMoveDir;

	// 1. C'e' un dislivello davanti? Una linea verso il basso, SearchDistance piu' avanti.
	const FVector Far = FVector(CapsuleLocation.X, CapsuleLocation.Y, FeetZ) + Dir * S.SearchDistance;
	const FVector FarEnd = Far - Up * (S.MaxDropHeight + 20.f);
	FHitResult FarHit;
	const bool bFarFloor = World->LineTraceSingleByChannel(FarHit, Far + Up * 20.f, FarEnd, RoomChannel, Params);
	if (ShouldDrawDebug(2))
		DrawDebugLine(World, Far + Up * 20.f, bFarFloor ? FVector(FarHit.ImpactPoint) : FarEnd, FColor::Orange, false, DebugLifeTime());
	if (!bFarFloor) return Fail(TEXT("troppo alto: si cade"));
	if (FeetZ - FarHit.ImpactPoint.Z < S.MinDropHeight) return Fail(TEXT("nessun dislivello davanti"));

	// 2. La faccia sotto lo spigolo: linea orizzontale 5 cm sotto i piedi, dal vuoto verso la capsula.
	//    Regge anche se siamo gia' oltre lo spigolo: la faccia e' dietro.
	const FVector FaceStart = Far - Up * 5.f;
	const FVector FaceEnd = FaceStart - Dir * (S.SearchDistance + Radius + 50.f);
	FHitResult FaceHit;
	const bool bFace = World->LineTraceSingleByChannel(FaceHit, FaceStart, FaceEnd, RoomChannel, Params) && !FaceHit.bStartPenetrating;
	if (ShouldDrawDebug(2))
		DrawDebugLine(World, FaceStart, bFace ? FVector(FaceHit.ImpactPoint) : FaceEnd, FColor::Cyan, false, DebugLifeTime());
	if (!bFace || FMath::Abs(FaceHit.ImpactNormal.Z) > MaxFaceNormalZ) return Fail(TEXT("nessuno spigolo verticale"));
	if (!IsTraversable(FaceHit)) return Fail(TEXT("spigolo escluso: fisica attiva, personaggio o tag"));

	const FVector Normal = FaceHit.ImpactNormal.GetSafeNormal2D();	// verso il vuoto
	if (FVector::DotProduct(Dir, Normal) < FMath::Cos(FMath::DegreesToRadians(S.MaxApproachAngle)))
		return Fail(TEXT("spigolo preso troppo di lato"));

	// 3. Cima: dall'alto verso il basso, 1 cm dentro la faccia, solo contro il muretto.
	UPrimitiveComponent* Ledge = FaceHit.GetComponent();
	FHitResult TopHit;
	if (!FindEdgeAlong(Ledge, FaceHit.ImpactPoint - Normal * FaceInset, Up, 60.f, TopHit) || TopHit.ImpactNormal.Z < MinLedgeTopNormalZ)
		return Fail(TEXT("cima dello spigolo non trovata"));

	// Lo spigolo sulla linea della capsula, perpendicolare al bordo: li' la clip ha l'interaction.
	const FVector Center2D(CapsuleLocation.X, CapsuleLocation.Y, 0.f);
	const FVector Top2D(TopHit.ImpactPoint.X, TopHit.ImpactPoint.Y, 0.f);
	Result.DistanceToEdge = FVector::DotProduct(Top2D - Center2D, Normal);
	Result.EdgeLocation = Center2D + Normal * Result.DistanceToEdge;
	Result.EdgeLocation.Z = TopHit.ImpactPoint.Z;
	Result.EdgeNormal = Normal;
	Result.LedgeComponent = Ledge;

	// 4. Pavimento d'arrivo, appena oltre lo spigolo: da' quota e dislivello.
	const FVector FloorProbe = Result.EdgeLocation + Normal * (Radius + RoomPadding);
	FHitResult FloorHit;
	if (!World->LineTraceSingleByChannel(FloorHit, FloorProbe - Up * 2.f, FloorProbe - Up * (S.MaxDropHeight + 10.f), RoomChannel, Params))
		return Fail(TEXT("troppo alto: si cade"));
	if (!CharacterMovement->IsWalkable(FloorHit)) return Fail(TEXT("pavimento d'arrivo non calpestabile"));
	Result.FloorLocation = FloorHit.ImpactPoint;
	Result.DropHeight = Result.EdgeLocation.Z - FloorHit.ImpactPoint.Z;

	if (ShouldDrawDebug(1))
	{
		DrawDebugSphere(World, Result.EdgeLocation, 10.f, 12, FColor::Green, false, DebugLifeTime());
		DrawDebugDirectionalArrow(World, Result.EdgeLocation, Result.EdgeLocation + Normal * 40.f, 15.f, FColor::Green, false, DebugLifeTime());
		DrawDebugSphere(World, Result.FloorLocation, 8.f, 12, FColor::Yellow, false, DebugLifeTime());
	}

	if (Result.DropHeight < S.MinDropHeight) return Fail(TEXT("dislivello basso: si scende camminando"));
	if (Result.DropHeight > S.MaxDropHeight) return Fail(TEXT("troppo alto: si cade"));

	// Finestra d'innesco: troppo lontani si aspetta. Nella finestra serve muoversi abbastanza per il commit;
	// sotto la soglia minima parte subito, anche da fermi.
	if (Result.DistanceToEdge > S.TriggerMaxDistance) return Fail(TEXT("spigolo ancora lontano"));
	Result.bForced = Result.DistanceToEdge < S.TriggerMinDistance;
	if (!Result.bForced)
	{
		if (bStill) return Fail(TEXT("fermo nella finestra"));
		if (Velocity2D.SizeSquared() < FMath::Square(S.CommitMinSpeed)) return Fail(TEXT("troppo lento per il commit"));
	}

	// 5. Montaggio e frame d'ingresso dalla chooser. Nessuna riga per questo dislivello: High, decide il chiamante.
	bool bTooFar = false;
	if (ChooseDropMontage(Result, bTooFar))
	{
		// 6. Punto d'arrivo (clip, ma staccato dal muro) e spazio li'. Raggio pieno del CDO:
		//    tornati in Walking la capsula si riallarga, anche se ora lo shoulder avoidance la stringe.
		if (!ComputeDropLanding(S, DefaultCapsuleRadius, Result)) return false;
		Result.Type = EDropDownType::Low;
		return true;
	}
	if (bTooFar) return false;

	Result.Type = EDropDownType::High;
	return true;
}

bool UTraversalComponent::ChooseDropMontage(FDropDownCheckResult& Result, bool& bOutTooFar) const
{
	bOutTooFar = false;

	// Stessa chooser e stesso Motion Match del traversal: la riga del drop-down si riconosce da bIsDropDown.
	// Lo spigolo arriva come un bordo con la normale verso il personaggio, cioe' come una faccia di muro.
	FTraversalChooserInputs Inputs;
	Inputs.bIsDropDown = true;
	Inputs.bHasFrontLedge = false;
	Inputs.ObstacleHeight = Result.DropHeight;
	Inputs.MovementMode = CharacterMovement->MovementMode;
	Inputs.Speed = CharacterMovement->Velocity.Size2D();

	ETraversalActionType ActionType = ETraversalActionType::None;
	UAnimMontage* Montage = nullptr;
	if (!EvaluateChooser(Inputs, Result.EdgeLocation, -Result.EdgeNormal, ActionType, Montage, Result.StartTime, Result.PlayRate))
		return false;
	if (ActionType != ETraversalActionType::DropDown) return Fail(TEXT("la chooser non ha risposto DropDown"));

	// Troppo lontani: la clip comincia piu' vicina allo spigolo. Chi chiama a ogni frame riprova al prossimo.
	float FirstDistance = 0.f;
	if (GetDropCurve(Montage, 0.f, FirstDistance) && Result.DistanceToEdge > FirstDistance)
	{
		bOutTooFar = true;
		return Fail(TEXT("spigolo ancora lontano"));
	}

	Result.Montage = Montage;
	return true;
}


bool UTraversalComponent::ComputeDropLanding(const FDropDownSettings& Settings, float Radius, FDropDownCheckResult& Result) const
{
	UWorld* World = GetWorld();
	if (!World || !Result.Montage) return false;

	const FVector N = FVector(Result.EdgeNormal.X, Result.EdgeNormal.Y, 0.f).GetSafeNormal();	// verso il vuoto
	const FVector Edge = Result.EdgeLocation;
	const float FloorZ = Result.FloorLocation.Z;

	// 1. Dove atterra la clip: distanza dallo spigolo a fine finestra DropFloor (positiva = oltre lo spigolo).
	//    La curva e' positiva prima dello spigolo e negativa dopo: si cambia segno.
	float AnimLand = 0.f;
	TArray<FMotionWarpingWindowData> Windows;
	UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(Result.Montage, DropFloorWarpName, Windows);
	if (!Windows.IsEmpty())
	{
		float CurveValue = 0.f;
		if (GetDropCurve(Result.Montage, Windows[0].EndTime, CurveValue)) AnimLand = -CurveValue;
	}
	const float MaxLand = FMath::Max(AnimLand, 0.f) + Settings.MaxLandingPush;

	// 2. Dove tocca davvero il muro, all'altezza dell'arrivo: la capsula accucciata parte dal punto
	//    piu' lontano consentito e scivola verso lo spigolo. Stessi canali della capsula, quindi
	//    trova quello che blocchera' la CMC quando si torna in Walking.
	const float HalfHeight = CharacterMovement->GetCrouchedHalfHeight();
	const float Lift = 10.f;	// sopra il pavimento, cosi' piastrelle e piccoli dislivelli non contano
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(DropLanding), false, GetOwner());
	const FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
	const ECollisionChannel Channel = Capsule->GetCollisionObjectType();

	const FVector EdgeAtLand(Edge.X, Edge.Y, FloorZ + HalfHeight + Lift);
	FHitResult WallHit;
	const bool bWall = World->SweepSingleByChannel(WallHit, EdgeAtLand + N * (MaxLand + 1.f), EdgeAtLand,
		FQuat::Identity, Channel, Shape, Params, Response);
	if (bWall && WallHit.bStartPenetrating) return Fail(TEXT("spazio d'arrivo occupato"));

	// Centro capsula quando tocca il muro, misurato dallo spigolo. Niente hit: il muro rientra, vale la clip.
	const float Touch = bWall ? static_cast<float>(FVector::DotProduct(WallHit.Location - EdgeAtLand, N)) : TNumericLimits<float>::Lowest();
	const float LandDist = FMath::Max(AnimLand, Touch + Settings.LandingClearance);
	if (LandDist > MaxLand) return Fail(TEXT("muro sotto troppo sporgente"));

	FVector Land = Edge + N * LandDist;
	Land.Z = FloorZ;

	// 3. Lo spazio si controlla dove si atterra davvero.
	const FVector RoomCenter = Land + FVector(0.f, 0.f, HalfHeight + Lift);
	const bool bBlocked = World->OverlapBlockingTestByChannel(RoomCenter, FQuat::Identity, Channel, Shape, Params, Response);
	DrawRoom(RoomCenter, Radius, HalfHeight, bBlocked);
	if (bBlocked) return Fail(TEXT("niente spazio all'arrivo"));

	Result.LandLocation = Land;

#if !UE_BUILD_SHIPPING
	if (ShouldDrawDebug(1))
	{
		DrawDebugSphere(World, Land, 8.f, 12, FColor::Blue, false, DebugLifeTime());
		if (bWall) DrawDebugPoint(World, WallHit.ImpactPoint, 12.f, FColor::Magenta, false, DebugLifeTime());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(54, DebugLifeTime(), FColor::Cyan, FString::Printf(
				TEXT("DropDown atterraggio a %.0f cm  (clip %.0f, contatto muro %.0f)"), LandDist, AnimLand, bWall ? Touch : 0.f));
		}
	}
#endif
	return true;
}

bool UTraversalComponent::GetDropCurve(const UAnimMontage* Montage, float Time, float& OutValue) const
{
	// La curva sta nella sequence del montaggio, dove la scrive lo script: si legge dal primo segmento.
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty()) return false;
	const FAnimSegment& Segment = Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
	const UAnimSequenceBase* Sequence = Segment.GetAnimReference();
	if (!Sequence) return false;
	return UAnimationWarpingLibrary::GetCurveValueFromAnimation(Sequence, DropDistanceCurveName, Segment.ConvertTrackPosToAnimPos(Time), OutValue);
}

bool UTraversalComponent::PerformDropDown(const FDropDownCheckResult& Result)
{
	UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!HostAnim || !Mesh || !CharacterMovement || !Capsule) return false;
	if (Result.Type != EDropDownType::Low || !Result.Montage || bDoingTraversal) return false;
	if (bDoingDropDown) AbortDropDown();

	ActiveDrop = Result;
	UpdateDropWarpTargets();

	// Dal frame e alla velocita' scelti dal Motion Match, come il traversal.
	if (HostAnim->Montage_Play(Result.Montage, Result.PlayRate, EMontagePlayReturnType::MontageLength, Result.StartTime) <= 0.f)
		return false;

	++DropSerial;
	FOnMontageBlendingOutStarted BlendingOut;
	BlendingOut.BindUObject(this, &UTraversalComponent::OnDropBlendingOut, DropSerial);
	HostAnim->Montage_SetBlendingOutDelegate(BlendingOut, Result.Montage);
	FOnMontageEnded Ended;
	Ended.BindUObject(this, &UTraversalComponent::OnDropEnded, DropSerial);
	HostAnim->Montage_SetEndDelegate(Ended, Result.Montage);

	// Prima del Flying: il CMC lo legge per restare accucciato (CanCrouchInCurrentState).
	bDoingDropDown = true;

	// La capsula scende passando oltre il bordo del muretto: la muove il root motion.
	if (Result.LedgeComponent) Capsule->IgnoreComponentWhenMoving(Result.LedgeComponent, true);
	SavedDropMeshResponse = Mesh->GetCollisionResponseToChannel(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);

	// Flying: il root motion muove anche in verticale. InAir guarda solo Falling, quindi non scatta.
	CharacterMovement->SetMovementMode(MOVE_Flying);
	return true;
}

void UTraversalComponent::UpdateDropWarpTargets()
{
	if (!MotionWarping || !ActiveDrop.Montage) return;
	const FDropDownCheckResult& R = ActiveDrop;

	// Stessa convenzione di FrontLedge: X lungo la marcia, che qui va dalla cima verso il vuoto.
	const FRotator Facing = FRotationMatrix::MakeFromX(R.EdgeNormal).Rotator();

	// DropLedge: l'interaction della clip sullo spigolo.
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(DropLedgeWarpName, R.EdgeLocation, Facing);

	// DropFloor: il root a terra nel punto di ComputeDropLanding. E' quello della clip,
	// ma mai piu' vicino al muro di raggio + LandingClearance.
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(DropFloorWarpName, R.LandLocation, Facing);
}

void UTraversalComponent::AbortDropDown()
{
	if (!bDoingDropDown) return;

	// Come AbortTraversal: prima si staccano i delegate, poi si ferma.
	UCustomAnimInstance* HostAnim = GetHostAnimInstance();
	if (HostAnim && ActiveDrop.Montage)
	{
		FOnMontageBlendingOutStarted NoBlendingOut;
		HostAnim->Montage_SetBlendingOutDelegate(NoBlendingOut, ActiveDrop.Montage);
		FOnMontageEnded NoEnded;
		HostAnim->Montage_SetEndDelegate(NoEnded, ActiveDrop.Montage);
		HostAnim->Montage_Stop(AbortBlendOutTime, ActiveDrop.Montage);
	}
	FinishDropDown(true, false);
}

void UTraversalComponent::OnDropBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 Serial)
{
	if (Serial != DropSerial) return;
	FinishDropDown(bInterrupted, true);
}

void UTraversalComponent::OnDropEnded(UAnimMontage* Montage, bool bInterrupted, int32 Serial)
{
	if (Serial != DropSerial) return;
	FinishDropDown(bInterrupted, true);	// di solito l'ha gia' chiuso il blend out
}

void UTraversalComponent::FinishDropDown(bool bInterrupted, bool bNotify)
{
	if (!bDoingDropDown) return;
	bDoingDropDown = false;

	if (Capsule && ActiveDrop.LedgeComponent) Capsule->IgnoreComponentWhenMoving(ActiveDrop.LedgeComponent, false);
	if (USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr)
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, SavedDropMeshResponse);

	// Come all'uscita del traversal: la capsula dove la vuole il CMC prima di Walking, senza scatti.
	float GroundZ = 0.f;
	if (GetGroundedCapsuleZ(GroundZ) && FMath::Abs(GroundZ - Character->GetActorLocation().Z) <= GroundSnapMaxDistance)
	{
		FVector Location = Character->GetActorLocation();
		Location.Z = GroundZ;
		Character->SetActorLocation(Location);
	}

	// Se e' stato interrotto a mezz'aria, il CMC passa a Falling da solo.
	if (CharacterMovement) CharacterMovement->SetMovementMode(MOVE_Walking);

	if (bNotify) OnDropDownFinished.Broadcast(bInterrupted);
}


/* ---> DEBUG
**/

bool UTraversalComponent::ShouldDrawDebug(int32 Level) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarTraversalDrawDebugLevel.GetValueOnGameThread() >= Level;
#endif
}

float UTraversalComponent::DebugLifeTime() const
{
	return bAutoChecking ? 0.f : CVarTraversalDrawDebugDuration.GetValueOnGameThread();
}

void UTraversalComponent::DrawLedges(const FTraversalCheckResult& Result) const
{
	if (!ShouldDrawDebug(1)) return;

	const UWorld* World = GetWorld();
	const float Life = DebugLifeTime();
	if (Result.bHasFrontLedge)
	{
		DrawDebugSphere(World, Result.FrontLedgeLocation, 10.f, 12, FColor::Green, false, Life);
		DrawDebugDirectionalArrow(World, Result.FrontLedgeLocation, Result.FrontLedgeLocation + Result.FrontLedgeNormal * 40.f,
			15.f, FColor::Green, false, Life);
	}
	if (Result.bHasBackLedge)
	{
		DrawDebugSphere(World, Result.BackLedgeLocation, 10.f, 12, FColor::Blue, false, Life);
		DrawDebugDirectionalArrow(World, Result.BackLedgeLocation, Result.BackLedgeLocation + Result.BackLedgeNormal * 40.f,
			15.f, FColor::Blue, false, Life);
	}
}

void UTraversalComponent::DrawRoom(const FVector& Center, float Radius, float HalfHeight, bool bBlocked) const
{
	if (ShouldDrawDebug(1))
		DrawDebugCapsule(GetWorld(), Center, HalfHeight, Radius, FQuat::Identity, bBlocked ? FColor::Red : FColor::Green, false, DebugLifeTime());
}

void UTraversalComponent::LogExitFrame(const TCHAR* Label) const
{
#if !UE_BUILD_SHIPPING
	if (!Character || !CharacterMovement) return;
	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	const FVector Cam = (PC && PC->PlayerCameraManager) ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const FVector Root = Mesh ? Mesh->GetBoneLocation(TEXT("root")) : FVector::ZeroVector;
	const FVector Pelvis = Mesh ? Mesh->GetBoneLocation(TEXT("pelvis")) : FVector::ZeroVector;

	UE_LOG(LogULS, Log, TEXT("Uscita %s | capsula %s | root %s | pelvis Z %.1f | camera %s | %s %s"),
		Label, *Character->GetActorLocation().ToCompactString(), *Root.ToCompactString(), Pelvis.Z,
		*Cam.ToCompactString(), *CharacterMovement->GetMovementName(), *CharacterMovement->Velocity.ToCompactString());
#endif
}