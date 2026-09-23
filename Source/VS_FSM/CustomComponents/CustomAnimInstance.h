// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
#include "CustomComponents/ShootingSystem/WeaponDataAsset.h"
#include "VS_FSMCharacter.h"
#include "Animation/AnimInstance.h"
#include "CustomAnimInstance.generated.h"

class UBlendSpace;
class ULocomotionStateComponent;

/**
 * Parent class di ABP_Host.
 *
 * NON è la parent dei layer: ABP_Loco_FSM e ABP_Loco_MotionMatching derivano da
 * UAnimInstance puro e leggono ULocomotionStateComponent. Se un layer ereditasse
 * da qui, GASP non potrebbe implementare lo stesso ALI senza essere reparentato.
 *
 * Qui resta solo ciò che l'host possiede davvero: il weapon system, che
 * UShootingSystem scrive via GetMesh()->GetAnimInstance() — chiamata che
 * ritorna l'host, mai un layer.
 *
 * Tutta la locomotion è su ULocomotionStateComponent. Se ti accorgi di stare
 * aggiungendo una variabile di locomotion in questo file, va sul componente.
 */
UCLASS()
class VS_FSM_API UCustomAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	/**
	 * Ponte per le notify che usano il magic naming e che girano su clip suonate
	 * dal layer di locomotion: inoltrano al componente.
	 * Perché funzionino dal layer serve "Propagate Notifies To Linked Instances"
	 * sul layer e "Receive Notifies From Linked Instances" sull'host (Class Settings).
	 * In alternativa, fai chiamare direttamente il componente da una UAnimNotify.
	 */
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ResetStanceTransition();

// ============================================================================
// WEAPON SYSTEM / AIM — futuro layer ALI_UpperBody, per ora vive nell'host.
// ============================================================================
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

protected:
	UPROPERTY(BlueprintReadOnly)
	AVS_FSMCharacter* PlayerRef = nullptr;

	/** Sorgente unica dei dati di locomotion. Qui serve solo per la stance. */
	UPROPERTY(BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<ULocomotionStateComponent> LocoComp = nullptr;

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	EStanceMode GetStanceMode() const;

private:
	/** Risolve LocoComp se manca. True se utilizzabile. */
	bool EnsureLocoComp();
};