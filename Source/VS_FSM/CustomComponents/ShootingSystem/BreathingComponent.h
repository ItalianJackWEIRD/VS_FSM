// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BreathingComponent.generated.h"

class UCustomAnimInstance;
class UShootingSystem;
class AVS_FSMCharacter;

enum class EPulsePhase : uint8 { Rest, Rising, Drifting, Falling};

/*
 *	This component, attached to the ThirdPersonCharacter, plays with the Height of the Blend1D for the Pistol Poses. The Alpha for the Bone Blend is handled inside 
 *	ShootingComponent, and here when stimulus or pulse is activated. When in rest NO.
 *	-> Rest, il componente tiene alpha e heigh a 0 e aspetta il timer per iniziare un altro PULSE oppure aspetta la chiamta esterna della Stimolo (che sovrasta tutto).
 *	Sceglie una posa (float) target e una posa start (se il target è 0 start rimane a 0, portato da falling, e si skippa il Rising).
 *	-> Rising, si porta il currentHeight da 0 a start con una velocità diversa.
 *	-> Drifting, viene interpolato height dal suo current (settato prima) fino al target e si tiene traccia del timer per il pulse
 *  -> Falling, viene impostato il target a 0 e quando lo tocca va in Rest.
 *  Alpha viene messo a 1 se non si sta nel Rest o se lo Stimolo è attivo. (Se in Alert gia va in 1 grazie a shooting component)
*/
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
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") float RiseSpeed = 6.f;
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") float DriftSpeed = 0.4f;
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") float FallSpeed = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") TArray<float> PulsePoseHeights = { 0.f, 0.5f, 1.f };
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Normal") FVector2D StartFractionRange = FVector2D(0.3f, 0.7f);
	
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Stimulus") float StimulusInterpSpeed = 2.5f;

	// Alert (per ora costante, i constraint li aggiungiamo domani)
	UPROPERTY(EditDefaultsOnly, Category="Breathing|Alert")  float     AlertHeight = 1.f;

private:
	void UpdateNormal(float Dt);
	void UpdateAlert(float Dt);

	float CurrentHeight = 0.f;
	float TargetHeight  = 0.f;

	// pulse runtime state
	float NextPulseTimer = 0.f;
	EPulsePhase PulsePhase = EPulsePhase::Rest;
	float PulseTarget = 0.f;
	float PulseStartPose = 0.f;
	float DriftRemaining = 0.f;
	float ActiveInterpSpeed = 4.f;

	// stimolo
	bool  bStimulusActive       = false;
	float StimulusHeight        = 0.f;
	float StimulusHoldRemaining = 0.f;
};