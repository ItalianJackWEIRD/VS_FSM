// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LocomotionStateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

ULocomotionStateComponent::ULocomotionStateComponent()
{

	PrimaryComponentTick.bCanEverTick = true;
	
	// Same as GASP
	TrajectoryDataIdle.RotateTowardsMovementSpeed = 0.f;
	TrajectoryDataIdle.MaxControllerYawRate = 100.f;
	TrajectoryDataMoving.RotateTowardsMovementSpeed = 0.f;
	TrajectoryDataMoving.MaxControllerYawRate = 0.f;
}

void ULocomotionStateComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (LocomotionType != ELocomotionBackend::Custom)	// Lascio la possibilità di settare un modello a propria scelta E a proprio rischio e pericolo!
	{
		const bool bFSM = LocomotionType == ELocomotionBackend::FSM;
		bEnableDistanceMatching = bFSM;
		bEnableIdleBreak = bFSM;
		bEnableIdleRecenter = bFSM;
		bEnablePivot = bFSM;
		bEnableShoulderVariants = bFSM;
		bEnableTurnInPlace = bFSM;
	}
	
	if (const AActor* Owner = GetOwner())
		CharacterMovement = Owner->FindComponentByClass<UCharacterMovementComponent>();
	
	if (CharacterMovement) AddTickPrerequisiteComponent(CharacterMovement);
	
	if (const ACharacter* Owner = Cast<ACharacter>(GetOwner()))
		if (USkeletalMeshComponent* Mesh = Owner->GetMesh())
			Mesh->AddTickPrerequisiteComponent(this);
}

void ULocomotionStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTrajectory(DeltaTime);
}

FVector ULocomotionStateComponent::GetAcceleration() const
{
	return CharacterMovement ? CharacterMovement->GetCurrentAcceleration() : FVector::ZeroVector;
}

void ULocomotionStateComponent::RefreshMovementCache()
{
	/*	scommenta se rompe qualcosa col distance matching
	if (!CharacterMovement)
	{
		if (const AActor* Owner = GetOwner())
			CharacterMovement = Owner->FindComponentByClass<UCharacterMovementComponent>();
	}
	*/
	if (!CharacterMovement || !bEnableDistanceMatching) return;

	bUseSeparateBrakingFriction = CharacterMovement->bUseSeparateBrakingFriction;
	BrakingFriction             = CharacterMovement->BrakingFriction;
	GroundFriction              = CharacterMovement->GroundFriction;
	BrakingFrictionFactor       = CharacterMovement->BrakingFrictionFactor;
	BrakingDecelerationWalking  = CharacterMovement->BrakingDecelerationWalking;
}

/* ---> TRIGGER ONE-SHOT
 * La FSM arma il flag, il grafo lo consuma leggendolo. Pure per non dover
 * cablare un exec pin in una transition rule.
**/

bool ULocomotionStateComponent::ShouldIdleBreak()
{
	if (bShouldIdleBreak)
	{
		bShouldIdleBreak = false;
		return true;
	}
	return false;
}

bool ULocomotionStateComponent::ShouldStanceTransition()
{
	if (bShouldStanceTransition)
	{
		bShouldStanceTransition = false;
		return true;
	}
	return false;
}

bool ULocomotionStateComponent::ShouldMovWalkJogStanceTransition()
{
	if (bShouldWalkJogStanceTransition)
	{
		bShouldWalkJogStanceTransition = false;
		return true;
	}
	return false;
}

void ULocomotionStateComponent::ResetStanceTransition()
{
	bIsInStanceTransition = false;
}

void ULocomotionStateComponent::UpdateTrajectory(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	const UAnimInstance* Context = Mesh ? Mesh->GetAnimInstance() : nullptr;
	
	if (!Context || !CharacterMovement) return;

	// Come in GASP: parametri diversi da fermo e in movimento.
	const FPoseSearchTrajectoryData& Data =
		CharacterMovement->Velocity.SizeSquared2D() > KINDA_SMALL_NUMBER ? TrajectoryDataMoving : TrajectoryDataIdle;

	UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
		Context, Data, DeltaTime,
		TrajectoryState, DesiredControllerYawLastUpdate, Trajectory,
		HistorySamplingInterval, HistoryCount, PredictionSamplingInterval, PredictionCount);
}
