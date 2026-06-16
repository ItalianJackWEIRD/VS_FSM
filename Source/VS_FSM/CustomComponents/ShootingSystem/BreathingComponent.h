// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BreathingComponent.generated.h"

class UCustomAnimInstance;
class UShootingSystem;
class AVS_FSMCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VS_FSM_API UBreathingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UBreathingComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Stimolo esterno (porte, AI, trigger) — la regola 2 del Normal.
	UFUNCTION(BlueprintCallable, Category="Breathing")
	void RequestRaise(float TargetHeight, float HoldSeconds);
	UFUNCTION(BlueprintCallable, Category="Breathing")
	void RequestLower();

protected:
	virtual void BeginPlay() override;

	UPROPERTY() TObjectPtr<UCustomAnimInstance> AnimInstance = nullptr;
	UPROPERTY() TObjectPtr<UShootingSystem>     Shooting     = nullptr;
	UPROPERTY() TObjectPtr<AVS_FSMCharacter>    Character    = nullptr;

	// Tuning Normal (pulse random)
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") FVector2D IntervalRange    = FVector2D(4.f, 9.f);
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") FVector2D HoldRange        = FVector2D(1.5f, 3.f);
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") FVector2D PulseHeightRange = FVector2D(0.4f, 0.8f);
	UPROPERTY(EditDefaultsOnly, Category="Breathing")        float     HeightInterpSpeed = 4.f;

	// Alert (per ora costante, i constraint li aggiungiamo domani)
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Alert")  float     AlertHeight = 1.f;

private:
	void UpdateNormal(float Dt);
	void UpdateAlert(float Dt);

	float CurrentHeight = 0.f;
	float TargetHeight  = 0.f;

	// pulse state
	float NextPulseTimer = 0.f;
	float HoldRemaining  = 0.f;
	float PulseHeight    = 0.f;
	bool  bPulseActive   = false;

	// stimolo
	bool  bStimulusActive       = false;
	float StimulusHeight        = 0.f;
	float StimulusHoldRemaining = 0.f;
};