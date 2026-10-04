// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/LocomotionTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/TrajectoryTypes.h"
#include "CustomAnimInstance.generated.h"

class UBlendSpace;
class ULocomotionStateComponent;

/**
 * Parent class di ABP_Host.
 *
 * NON è la parent dei layer: ABP_Loco_FSM e ABP_Loco_MM derivano da UAnimInstance
 * puro e leggono ULocomotionStateComponent. Se un layer ereditasse da qui, GASP non
 * potrebbe implementare lo stesso ALI senza essere reparentato.
 *
 * Qui resta solo ciò che l'host possiede davvero: il weapon system, che
 * UShootingSystem scrive via GetMesh()->GetAnimInstance() — chiamata che
 * ritorna l'host, mai un layer.
 *
 * Tutta la locomotion è su ULocomotionStateComponent. Qui ci sono solo COPIE dei dati
 * che legge il grafo dell'host, fatte sul game thread in NativeUpdateAnimation, come
 * nei layer. Se ti accorgi di stare aggiungendo una variabile di locomotion che non è
 * una copia, va sul componente.
 */
UCLASS()
class ULS_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/**
	 * Ponte per le notify che usano il magic naming e che girano su clip suonate
	 * dal layer di locomotion: inoltrano al componente.
	 * Perché funzionino dal layer serve "Propagate Notifies To Linked Instances"
	 * sul layer e "Receive Notifies From Linked Instances" sull'host (Class Settings).
	 * In alternativa, fai chiamare direttamente il componente da una UAnimNotify.
	 */
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();

#pragma region LOCOMOTION_COPY

	/** Copia della traiettoria del componente: la legge la Pose History, con entrambi i backend. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	FTransformTrajectory Trajectory;

#pragma endregion
		
// ============================================================================
// WEAPON SYSTEM / AIM — futuro layer ALI_UpperBody, per ora vive nell'host.
// ============================================================================
	/*
#pragma region WEAPON

	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpperBodyOn = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpper1H = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	bool bUpper2H = false;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float Weapon1hAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float Weapon2hAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	float GripAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon System")
	EWeaponGrip WeaponGrip = EWeaponGrip::OneHand;

	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay1HStand = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay1HCrouch = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay2HStand = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UBlendSpace> Overlay2HCrouch = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	TObjectPtr<UAnimSequence> EquipUnEquipAnim = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Overlay")
	float OverlayHeight = 1.f;
	UPROPERTY(BlueprintReadWrite, Category="Weapon|Overlay")
	bool bShouldEquipWeapon = false;

	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	TObjectPtr<UAnimSequence> FinalAimPose = nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimAlpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimPitch = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Aim")
	float AimLeanAngle = 0.f;


#pragma endregion
*/

protected:

	/** Sorgente unica dei dati di locomotion. Il grafo dovrebbe leggere solo le copie. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	/** Copia locale: niente puntatori esterni dereferenziati da un worker thread. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	EStanceMode GetStanceMode() const { return StanceMode; }

	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	EStanceMode StanceMode = EStanceMode::Normal;
	
	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	float LegIKAlpha = 1.f;

private:
	/** Risolve LocoComp se manca. True solo se utilizzabile. */
	bool EnsureLocoComp();
	void PullFromComponent();
	
	
	/** Warning una volta sola: EnsureLocoComp gira ogni frame. */
	bool bWarnedMissingComp = false;
};