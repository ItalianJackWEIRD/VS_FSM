// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IkContextTypes.generated.h"

class ULookTargetComponent;

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
	float ReachDurationWalk = 0.35f;

	UPROPERTY(EditAnywhere, Category="Timing")
	float ReleaseDuration = 0.35f;
	
	UPROPERTY(EditAnywhere, Category="Timing")
	float ReleaseDurationWalk = 0.35f;

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
	
	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="// for walk."))
	float ReachTargetSpeedWalk = 15.f;

	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="How fast the planted hand slides along the wall: low = more friction."))
	float ContactSlideSpeed = 2.f;
	
	UPROPERTY(EditAnywhere, Category="Contact", meta=(ToolTip="// for walk."))
	float ContactSlideSpeedWalk = 2.f;

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

/** ===========
 * HEAD
 * ============ */

/** Output channel for the head. Written only by UIkContextComponent, copied by the host. */
USTRUCT(BlueprintType)
struct ULS_API FIkHeadChannel
{
	GENERATED_BODY()

	/** Rotations to add in component space, split along the neck. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FRotator Neck01 = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FRotator Neck02 = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	FRotator Head = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IkContext")
	float Alpha = 0.f;
};

/** Head look: the head follows a look direction within limits (the camera now, external targets later). */
USTRUCT(BlueprintType)
struct ULS_API FHeadLookSettings
{
	GENERATED_BODY()

	// --- Limits, degrees, relative to the character
	UPROPERTY(EditAnywhere, Category="Limits")
	float MaxYaw = 70.f;

	UPROPERTY(EditAnywhere, Category="Limits", meta=(ToolTip="Beyond this yaw the target is behind us: the head goes back to neutral instead of sticking to the limit."))
	float GiveUpYaw = 110.f;

	UPROPERTY(EditAnywhere, Category="Limits")
	float MaxPitchUp = 35.f;

	UPROPERTY(EditAnywhere, Category="Limits")
	float MaxPitchDown = 45.f;

	// --- Camera
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ToolTip="Distance of the look point along the camera ray."))
	float CameraLookDistance = 1000.f;

	// --- Speeds
	UPROPERTY(EditAnywhere, Category="Speed", meta=(ToolTip="How fast yaw and pitch follow the target."))
	float FollowSpeed = 6.f;

	UPROPERTY(EditAnywhere, Category="Speed")
	float AlphaInSpeed = 4.f;

	UPROPERTY(EditAnywhere, Category="Speed")
	float AlphaOutSpeed = 3.f;

	// --- Split along the neck, normalized at runtime
	UPROPERTY(EditAnywhere, Category="Split", meta=(ClampMin="0"))
	float Neck01Weight = 0.25f;

	UPROPERTY(EditAnywhere, Category="Split", meta=(ClampMin="0"))
	float Neck02Weight = 0.25f;

	UPROPERTY(EditAnywhere, Category="Split", meta=(ClampMin="0"))
	float HeadWeight = 0.5f;
	
	// --- Targets (head API)
	UPROPERTY(EditAnywhere, Category="Targets", meta=(ToolTip="Follow speed on targets and right after any switch: low = the look glides."))
	float TargetFollowSpeed = 2.5f;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(ToolTip="After a switch the slow speed is used for this long, also when going back to the camera."))
	float SwitchBlendTime = 0.6f;

	UPROPERTY(EditAnywhere, Category="Targets")
	float TargetAlphaInSpeed = 2.5f;

	UPROPERTY(EditAnywhere, Category="Targets")
	float TargetAlphaOutSpeed = 1.5f;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(ClampMin="0.1", ClampMax="1.0", ToolTip="Same priority: a new target steals the look only if closer than this fraction of the current distance."))
	float TargetSwitchMargin = 0.8f;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(ToolTip="How often the targets are re-evaluated, seconds."))
	float SelectionInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(ToolTip="A target behind a wall is not looked at."))
	bool bCheckLineOfSight = true;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(ToolTip="After a while the character looks away and leaves that target alone for a bit."))
	bool bUseInterest = true;

	UPROPERTY(EditAnywhere, Category="Targets", meta=(EditCondition="bUseInterest", ToolTip="Random look time on one target, min-max seconds."))
	FVector2D InterestDuration = FVector2D(2.5f, 5.f);

	UPROPERTY(EditAnywhere, Category="Targets", meta=(EditCondition="bUseInterest", ToolTip="Random rest time before the same target can be looked at again, min-max seconds."))
	FVector2D InterestCooldown = FVector2D(4.f, 8.f);
	
	// --- Bones
	UPROPERTY(EditAnywhere, Category="Bones", meta=(ToolTip="Origin of the look direction."))
	FName HeadBone = TEXT("head");
};

/** Where the look comes from. */
enum class EHeadLookSource : uint8
{
	None,
	Camera,
	Target
};

/** One registered look target. Not reflected: only the component reads and writes it. */
struct FLookTargetEntry
{
	TWeakObjectPtr<ULookTargetComponent> Target;
	float CooldownUntil = 0.f;   // world time, interest cooldown
};

/** Head look runtime state. Not reflected: only the component reads and writes it. */
struct FHeadLookRuntime
{
	float Yaw = 0.f;    // degrees, relative to the character
	float Pitch = 0.f;
	float Alpha = 0.f;
	
	EHeadLookSource Source = EHeadLookSource::None;
	float SwitchTime = 100.f;    // seconds since the last switch; high at start
	bool bLastWasTarget = false; // the last real source, for the fade out speed
};