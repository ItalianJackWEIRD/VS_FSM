// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerBaseState.h"
#include "CustomComponents/CustomPlayerControllerInterface.h"
#include "DataAsset/LocomotionDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UPlayerBaseState::OnEnterState(AActor* OwnerRef)
{	
	//Save player ref for later
	if (!PlayerRef)
		PlayerRef = Cast<AVS_FSMCharacter>(OwnerRef);
	
	//Save CMC
	if (!CharacterMovementComponent && PlayerRef)
		CharacterMovementComponent = PlayerRef->GetCharacterMovement();
	
	//Save PlayerController*
	if (!PlayerController)
		PlayerController = Cast<ICustomPlayerControllerInterface>(UGameplayStatics::GetPlayerController(this, 0));
	
	//Save CustomAnimInstance
	if (!AnimInstance && PlayerRef)
		AnimInstance = Cast<UCustomAnimInstance>(PlayerRef->GetMesh()->GetAnimInstance());
	
	//Save CameraComponent
	if (!CameraRef && PlayerRef)
		CameraRef = Cast<UVSCameraComponent>(PlayerRef->FindComponentByClass<UVSCameraComponent>());
	
	//Bind Delegates
	SetupDelegates();
	
	//Import State Data and Refresh in Custom Anim Instance
	if (IsValid(StateData) && CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = StateData->MovementSpeed;
		CharacterMovementComponent->MaxAcceleration = StateData->MaxAcceleration;
		CharacterMovementComponent->BrakingDecelerationWalking = StateData->BrakingDeceleration;
		CharacterMovementComponent->BrakingFrictionFactor = StateData->BrakingFrictionFactor;
		CharacterMovementComponent->BrakingFriction = StateData->BrakingFriction;
		CharacterMovementComponent->bUseSeparateBrakingFriction = StateData->bUseSeparateBrakingFriction;
		CharacterMovementComponent->RotationRate = FRotator(0.f, StateData->RotationRate, 0.f);
		
		AnimInstance->StateIndex = StateData->StateIndex;
		AnimInstance->RefreshDataAsset();
	}
}

void UPlayerBaseState::OnExitState()
{
	//Super::OnExitState();		--> empty
	
	ResetDelegates();
}

void UPlayerBaseState::SetupDelegates()
{
	PlayerController->GetJumpDelegate()->AddUObject(this, &UPlayerBaseState::OnJump);
	PlayerController->GetCrouchDelegate()->AddUObject(this, &UPlayerBaseState::OnCrouch);
}

void UPlayerBaseState::ResetDelegates()
{
	PlayerController->GetJumpDelegate()->RemoveAll(this);
	PlayerController->GetCrouchDelegate()->RemoveAll(this);
}

bool UPlayerBaseState::IsEnemy(const AActor* Actor) const
{
	if (!IsValid(Actor)) { return false; }
	return Actor->ActorHasTag(EnemyTag);
}

void UPlayerBaseState::TickState(float DeltaTime)
{
	Super::TickState(DeltaTime);
	
	if (!AnimInstance) { return; }
	
	if (AnimInstance->TimerEnemyPoll > PollInterval)
	{
		AnimInstance->TimerEnemyPoll = 0.f;
		bool bEnemyDetected = false;
		
		if (PlayerRef)
		{
			TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
			ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

			TArray<AActor*> ToIgnore { PlayerRef };
			TArray<AActor*> Found;

			UKismetSystemLibrary::SphereOverlapActors(
				PlayerRef, PlayerRef->GetActorLocation(), DetectionRadius,
				ObjectTypes, nullptr, ToIgnore, Found);

			for (const AActor* A : Found)
			{
				if (IsEnemy(A)) { bEnemyDetected = true; break; }
			}
		}
		AnimInstance->bEnemyDetected = bEnemyDetected;
	}
	AnimInstance->TimerEnemyPoll += DeltaTime;
}


// To override
void UPlayerBaseState::OnJump()
{
}

void UPlayerBaseState::OnCrouch()
{
}


