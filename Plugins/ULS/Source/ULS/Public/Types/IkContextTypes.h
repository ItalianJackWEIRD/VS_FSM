// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IkContextTypes.generated.h"

/** Which arm pose the host applies before the arm IK. */
UENUM(BlueprintType)
enum class EIkArmPose : uint8
{
	None,
	WallTouch
};

/**
 * Output channel for one arm. Written only by UIkContextComponent, copied by the host.
 * Every arm mechanic (wall touch now, wall brace later) feeds this same channel.
 */
USTRUCT(BlueprintType)
struct ULS_API FIkArmChannel
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	EIkArmPose Pose = EIkArmPose::None;

	/** Alpha of the additive arm pose. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	float PoseAlpha = 0.f;

	/** Alpha of the arm IK. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	float IKAlpha = 0.f;

	/** Hand target, world space. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FVector TargetLocation = FVector::ZeroVector;

	/** Rotation facing the surface, world space (X into the surface, Z up). The ABP adds the per-bone offset. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FRotator TargetRotation = FRotator::ZeroRotator;

	/** Elbow pole, world space. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FVector JointTargetLocation = FVector::ZeroVector;

	void Reset() { *this = FIkArmChannel(); }
};

UENUM(BlueprintType)
enum class EWallTouchState : uint8
{
	Inactive,
	Reach,
	Contact,
	Release,
	Cooldown
};

/** Wall touch: every now and then, while walking or jogging, the hand pushes on the wall. */
USTRUCT(BlueprintType)
struct ULS_API FWallTouchSettings
{
	GENERATED_BODY()

	// --- Probe
	UPROPERTY(EditAnywhere, Category="Probe")
	float ProbeRadius = 8.f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="How far below the shoulder the hand touches the wall."))
	float HandHeightBelowShoulder = 20.f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="How far ahead of the shoulder the hand lands."))
	float LeadDistance = 15.f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ClampMin="0.5", ClampMax="1.0", ToolTip="Max reach as a fraction of the arm length."))
	float MaxReachRatio = 0.95f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="Closer than this to the shoulder the arm would be cramped."))
	float MinShoulderDistance = 28.f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="Max |Z| of the wall normal: above this it is a floor or a ceiling."))
	float MaxNormalZ = 0.3f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="Min dot between the wall normal and the direction back to us."))
	float MinFacing = 0.6f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="Hand target distance from the wall surface."))
	float PalmOffset = 3.f;

	UPROPERTY(EditAnywhere, Category="Probe", meta=(ToolTip="Below this speed no push starts, and a running one is released."))
	float MinSpeed = 50.f;

	// --- Timing
	UPROPERTY(EditAnywhere, Category="Timing")
	float ReachDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category="Timing")
	float ReleaseDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category="Timing", meta=(ClampMin="0.1", ClampMax="1.0", ToolTip="Reach: the pose is fully in at this fraction of the timeline."))
	float PoseLeadEnd = 0.6f;

	UPROPERTY(EditAnywhere, Category="Timing", meta=(ClampMin="0.0", ClampMax="0.9", ToolTip="Reach: the IK starts at this fraction, while the pose is still rising."))
	float IKStart = 0.25f;

	UPROPERTY(EditAnywhere, Category="Timing", meta=(ToolTip="How long the target can be missing (door frames, gaps) before releasing."))
	float TargetLostGrace = 0.15f;

	UPROPERTY(EditAnywhere, Category="Timing", meta=(ToolTip="Random max duration of a push, min-max seconds."))
	FVector2D ContactDuration = FVector2D(0.25f, 0.6f);

	UPROPERTY(EditAnywhere, Category="Timing", meta=(ToolTip="Random pause between two pushes, min-max seconds."))
	FVector2D Cooldown = FVector2D(1.5f, 4.f);

	// --- Contact
	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="How fast the target follows the wall point while reaching."))
	float ReachTargetSpeed = 15.f;

	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="How fast the planted hand slides along the wall: low = more friction."))
	float ContactSlideSpeed = 2.f;

	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="The push ends when the hand is this far behind the shoulder."))
	float BehindLimit = 15.f;

	// --- Elbow pole, from the shoulder
	UPROPERTY(EditAnywhere, Category="Elbow")
	float ElbowPoleDown = 40.f;

	UPROPERTY(EditAnywhere, Category="Elbow")
	float ElbowPoleBack = 25.f;

	UPROPERTY(EditAnywhere, Category="Elbow", meta=(ToolTip="Toward the wall."))
	float ElbowPoleOut = 10.f;

	// --- Bones
	UPROPERTY(EditAnywhere, Category="Bones") FName UpperArmL = TEXT("upperarm_l");
	UPROPERTY(EditAnywhere, Category="Bones") FName LowerArmL = TEXT("lowerarm_l");
	UPROPERTY(EditAnywhere, Category="Bones") FName HandL     = TEXT("hand_l");
	UPROPERTY(EditAnywhere, Category="Bones") FName UpperArmR = TEXT("upperarm_r");
	UPROPERTY(EditAnywhere, Category="Bones") FName LowerArmR = TEXT("lowerarm_r");
	UPROPERTY(EditAnywhere, Category="Bones") FName HandR     = TEXT("hand_r");
};

/** Wall touch runtime state. Not reflected: only the component reads and writes it. */
struct FWallTouchRuntime
{
	EWallTouchState State = EWallTouchState::Inactive;
	int32 Side = 0;              // -1 left, +1 right, 0 none
	float StateTime = 0.f;       // seconds in the current state
	float Duration = 0.f;        // random duration of Contact and Cooldown
	float LostTime = 0.f;        // how long the target has been invalid
	float PoseAlpha = 0.f;
	float IKAlpha = 0.f;
	float ReleasePoseFrom = 0.f; // the release starts from the current values, not from 1
	float ReleaseIKFrom = 0.f;
	FVector Forward = FVector::ForwardVector;
	FVector Target = FVector::ZeroVector;      // smoothed hand target, world
	FVector Anchor = FVector::ZeroVector;      // planted point during the push, world
	FVector WallNormal = FVector::ForwardVector;
};