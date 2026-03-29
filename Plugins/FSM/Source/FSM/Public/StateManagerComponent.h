// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StateBase.h"
#include "StateManagerComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FSM_API UStateManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStateManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State Machine")
	FString InitialState;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State Machine")
	TMap<FString, TSubclassOf<UStateBase>> AvaibleStates;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "State Machine Debug")
	bool bDebug = false;

	UPROPERTY(BlueprintReadOnly)
	TArray<UStateBase*> StateHistory;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "State Machine Debug", meta = (ClampMin = "0", ClampMax = "10", UIMin = "10", UIMax = "10"))
	int32 StateHistoryLenght;

	UPROPERTY(BlueprintReadOnly)
	UStateBase* CurrentState = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UStateBase*> StateMap; // in beginplay , the component will create an instance of each state class in AvaibleStates and store them here for easy access


	UFUNCTION(BlueprintCallable, Category = "State Machine")
	void SwitchStateByKey(FString StateKey);
	UFUNCTION(BlueprintCallable, Category = "State Machine")
	void SwitchState(UStateBase* State);
	UFUNCTION(BlueprintCallable, Category = "State Machine")
	void InitStateManager();	//call on begin play


private:
	bool bCanTickState = false;
	void InitializeStates();

};
