// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

// Sets default values for this component's properties
UGrabber::UGrabber() :
MaxGrabDistance(1000.0f),
GrabRadius(100),
HoldDistance(600),
bHasGrabbable(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGrabber::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UPhysicsHandleComponent* PhysicsHandle = GetPhysicsHandle();
	if (PhysicsHandle == nullptr)
	{
		return;
	}

	if (PhysicsHandle->GetGrabbedComponent() != nullptr)
	{
		FVector TargetLocation = GetComponentLocation() + GetForwardVector() * HoldDistance;
		PhysicsHandle->SetTargetLocationAndRotation(TargetLocation, GetComponentRotation());
	}
}

void UGrabber::Grab()
{
	UPhysicsHandleComponent* PhysicsHandle = GetPhysicsHandle();
	if (PhysicsHandle == nullptr)
	{
		return;
	}

	FHitResult HitResult;

	bool bHasHit = GetGrabbableInreach(HitResult);
	if (bHasHit && bHasGrabbable == false)
	{
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();

		// Wake Rigid bodies from hit component
		HitComponent->WakeAllRigidBodies();
		PhysicsHandle->GrabComponentAtLocationWithRotation(HitComponent, NAME_None, HitResult.ImpactPoint, GetComponentRotation());

		bHasGrabbable = true;
	}
	// Release grabbed component
	else if (PhysicsHandle->GetGrabbedComponent() != nullptr && bHasGrabbable == true) 
	{
		UPrimitiveComponent* GrabbedComponent = PhysicsHandle->GetGrabbedComponent();

		// Apply impulse force forward to the grabbed component on release
		if (GrabbedComponent->IsSimulatingPhysics()) // Check if the grabbed component is simulating physics
		{
			FVector ImpulseDirection = PhysicsHandle->GetOwner()->GetActorForwardVector();
			float ImpulseStrength = 2000.0f; 

			GrabbedComponent->AddImpulse(ImpulseDirection * ImpulseStrength, NAME_None, true);
		}
		PhysicsHandle->GetGrabbedComponent()->WakeAllRigidBodies();
		PhysicsHandle->ReleaseComponent();
		bHasGrabbable = false;
	}
}

void UGrabber::Released()
{
	/*UPhysicsHandleComponent* PhysicsHandle = GetPhysicsHandle();
	if (PhysicsHandle == nullptr)
	{
		return;
	}*/

	// Release grabbed component
	/*if (PhysicsHandle->GetGrabbedComponent() != nullptr)
	{
		PhysicsHandle->GetGrabbedComponent()->WakeAllRigidBodies();
		PhysicsHandle->ReleaseComponent();
	}*/


	
}

void UGrabber::RotateX()
{
	UPhysicsHandleComponent* PhysicsHandle = GetPhysicsHandle();
	if (PhysicsHandle == nullptr || !PhysicsHandle->GetGrabbedComponent())
	{
		return;
	}
	FTransform TargetTransform = PhysicsHandle->TargetTransform;
	PhysicsHandle->bRotationConstrained = false;
	// New rotation by adding a 90-degree turn around the X-axis
	FRotator NewRotation = FRotator(TargetTransform.GetRotation().Rotator().Pitch + 90.0f, TargetTransform.GetRotation().Rotator().Yaw, TargetTransform.GetRotation().Rotator().Roll);
	TargetTransform.SetRotation(NewRotation.Quaternion());
	PhysicsHandle->TargetTransform = TargetTransform;

	//FVector TargetLocation;
	//FRotator TargetRotation;
	//PhysicsHandle->GetTargetLocationAndRotation(TargetLocation, TargetRotation);
	// New rotation by adding a 90-degree turn around the X-axis
	//FRotator NewRotation = FRotator(TargetRotation.Pitch + 90.0f, TargetRotation.Yaw, TargetRotation.Roll);
	// Set the new rotation
	//PhysicsHandle->SetTargetRotation(NewRotation);
}

void UGrabber::RotateZ()
{
	UPhysicsHandleComponent* PhysicsHandle = GetPhysicsHandle();
	if (PhysicsHandle == nullptr || !PhysicsHandle->GetGrabbedComponent())
	{
		return;
	}
	FTransform TargetTransform = PhysicsHandle->TargetTransform;
	// New rotation by adding a 90-degree turn around the Z-axis
	FRotator NewRotation = FRotator(TargetTransform.GetRotation().Rotator().Pitch, TargetTransform.GetRotation().Rotator().Yaw, TargetTransform.GetRotation().Rotator().Roll + 90.0f);
	TargetTransform.SetRotation(NewRotation.Quaternion());
	PhysicsHandle->TargetTransform = TargetTransform;
}


UPhysicsHandleComponent* UGrabber::GetPhysicsHandle() const
{
	UPhysicsHandleComponent* Result = GetOwner()->FindComponentByClass<UPhysicsHandleComponent>();

	if (Result == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT(" Grabber requires a UPhysicsHandleComponent"));
	}
	return Result;
}

bool UGrabber::GetGrabbableInreach(FHitResult& OutHitResult) const
{
	FVector	Start = GetComponentLocation();
	FVector End = Start + GetForwardVector() * MaxGrabDistance;
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red);
	//DrawDebugSphere(GetWorld(), End, 50, 10, FColor::Blue, false, 5);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(GrabRadius);
	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, Sphere);;
}



