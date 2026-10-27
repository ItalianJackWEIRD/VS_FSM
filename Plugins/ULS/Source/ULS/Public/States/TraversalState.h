// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States/PlayerBaseState.h"
#include "TraversalState.generated.h"

class UTraversalComponent;

/**
 * Traversal (hurdle, vault, mantle). Il componente propone, la StateManager decide, questo stato esegue.
 *
 * Ci si entra solo da TryEnterTraversal, che ha già scelto l'azione: OnEnter la fa partire.
 * Esce quando il componente avvisa la fine: in InAir se si sta cadendo (il vault), altrimenti in Idle,
 * che passa da sé a Walk o Jog se ti muovi. Un watchdog impedisce che un montaggio senza fine blocchi il personaggio.
 *
 * Non sovrascrive OnTraversal né OnCrouch: durante il traversal quegli input non fanno niente.
 */
UCLASS()
class ULS_API UTraversalState : public UPlayerBaseState
{
	GENERATED_BODY()

public:
	virtual void OnEnterState(AActor* OwnerRef) override;
	virtual void OnExitState() override;
	virtual void TickState(float DeltaTime) override;

protected:
	// Il Falling di fine vault non porta in InAir da solo: l'uscita la decide ExitTraversal.
	virtual void OnStartFalling() override {}

	void HandleTraversalFinished(bool bInterrupted);
	void ExitTraversal();
	
	/** Lo stato di terra per come si muove la capsula adesso: le regole degli switch di Idle, Walk e Jog in un colpo solo. */
	FString GetGroundStateKey() const;
	
	UPROPERTY()
	TObjectPtr<UTraversalComponent> Traversal = nullptr;

	float Elapsed = 0.f;
	float MaxDuration = 4.f;
	bool bExitRequested = false;
};