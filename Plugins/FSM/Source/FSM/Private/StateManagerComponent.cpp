// Fill out your copyright notice in the Description page of Project Settings.


#include "StateManagerComponent.h"

// Sets default values for this component's properties
UStateManagerComponent::UStateManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UStateManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeStates();	// create states and hold them in memory (PRIVATE)
	// remember to call InitStateManager in the blueprint or it won't switch to the initial state
}


// Called every frame
void UStateManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCanTickState && CurrentState)
	{
		CurrentState->TickState(DeltaTime);
	}

	// Debug view
	if (bDebug)
	{
		if (CurrentState)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				0.f,
				FColor::Green,
				this->GetOwner()->GetName() + TEXT("'s current state: ") + CurrentState->StateDisplayName.GetPlainNameString(),
				true
			);
		}
		if (StateHistory.Num() > 0)
		{
			FString HistoryString = this->GetOwner()->GetName() + TEXT("'s state history: ");
			for (int32 i = StateHistory.Num() - 1; i >= 0; i--)
			{
				HistoryString += StateHistory[i]->StateDisplayName.GetPlainNameString();
				if (i != 0)
				{
					HistoryString += TEXT(" -> ");
				}
			}
			GEngine->AddOnScreenDebugMessage(
				-1,
				0.f,
				FColor::Yellow,
				HistoryString,
				true
			);
		}

	}

}



void UStateManagerComponent::SwitchStateByKey(FString StateKey)
{
	// Get Owner, since it's used 3 times
	AActor* Owner = this->GetOwner();
	// Bind the State
	UStateBase* NewState = StateMap.FindRef(StateKey);

	// Check if the state exists
	if (!IsValid(NewState))
	{
		if (bDebug && Owner)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Red,
				Owner->GetName() + TEXT("'s state switch failed. State with key: ") + StateKey + TEXT(" not found!"),
				true
			);
		}
		return;
	}

	// Prevent switching to the same state if it's not allowed
	if (CurrentState &&
		CurrentState->GetClass() == NewState->GetClass() &&
		!CurrentState->bCanRepeatState)
	{
		if (bDebug && Owner)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Red,
				Owner->GetName() + TEXT("'s state switch failed. ") +
				CurrentState->StateDisplayName.GetPlainNameString() +
				TEXT(" is not repeatable!"),
				true
			);
		}
		return;
	}

	// Exit current state and save history
	if (CurrentState)
	{
		bCanTickState = false;
		CurrentState->OnExitState();

		if (StateHistory.Num() >= StateHistoryLength)
		{
			StateHistory.RemoveAt(0);
		}
		StateHistory.Push(CurrentState);
	}

	CurrentState = NewState;
	CurrentState->OnEnterState(Owner);
	bCanTickState = true;

}


void UStateManagerComponent::SwitchState(UStateBase* State)
{
	// Get Owner, since it's used 3 times
	AActor* Owner = this->GetOwner();

	// Check if the state exists
	if (!IsValid(State))
	{
		if (bDebug && Owner)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Red,
				Owner->GetName() + TEXT("'s state switch failed. Invalid State passed!"),
				true
			);
		}
		return;
	}

	// Prevent switching to the same state if it's not allowed
	if (CurrentState &&
		CurrentState->GetClass() == State->GetClass() &&
		!CurrentState->bCanRepeatState)
	{
		if (bDebug && Owner)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Red,
				Owner->GetName() + TEXT("'s state switch failed. ") +
				CurrentState->StateDisplayName.GetPlainNameString() +
				TEXT(" is not repeatable!"),
				true
			);
		}
		return;
	}

	// Exit current state and save history
	if (CurrentState)
	{
		bCanTickState = false;
		CurrentState->OnExitState();

		if (StateHistory.Num() >= StateHistoryLength)
		{
			StateHistory.RemoveAt(0);
		}
		StateHistory.Push(CurrentState);
	}

	CurrentState = State;
	CurrentState->OnEnterState(Owner);
	bCanTickState = true;

}


void UStateManagerComponent::InitStateManager()
{
	SwitchStateByKey(InitialState);
}


void UStateManagerComponent::InitializeStates()
{
	// create states and hold them in memory
	for (auto It = AvaibleStates.CreateConstIterator(); It; ++It)	// It is ReadOnly
	{
		UStateBase* State = NewObject<UStateBase>(this, It->Value);
		StateMap.Add(It->Key, State);
	}
}

