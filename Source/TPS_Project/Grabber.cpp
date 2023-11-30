// Fill out your copyright notice in the Description page of Project Settings.


#include "Grabber.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "PlayerCharacter.h"

// Sets default values for this component's properties
UGrabber::UGrabber() :
MaxGrabDistance(1000.0f),
GrabRadius(100),
HoldDistance(600),
bHasGrabbable(false),
TotalRotation (0.0f)
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
	
}

void UGrabber::RotateX()
{
	if (bHasGrabbable)
	{
		// Stop player movement (Assuming APlayerCharacter has a function to stop movement)
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
		if (PlayerCharacter)
		{
			PlayerCharacter->bCanMove = false;
		}

		// Turn off physics in the grabbed component
		UPrimitiveComponent* GrabbedComponent = GetPhysicsHandle()->GetGrabbedComponent();
		if (GrabbedComponent)
		{
			GrabbedComponent->SetSimulatePhysics(false);
		}

		// Release from the physics handler
		GetPhysicsHandle()->ReleaseComponent();
		bHasGrabbable = false;

		// Rotate the grabbed component 90 degrees in the X-axis
		FRotator CurrentRotation = GrabbedComponent->GetComponentRotation();
		// Rotate the grabbed component by adding 90 degrees to the total rotation
		TotalRotation += 90.0f;

		// Ensure total rotation is within the range of 0 to 360 degrees
		TotalRotation = FMath::Fmod(TotalRotation, 360.0f);

		FRotator NewRotation = FRotator(TotalRotation, CurrentRotation.Yaw, CurrentRotation.Roll);
		GrabbedComponent->SetWorldRotation(NewRotation);

		// Grab the component again
		Grab();

		// Turn on necessary flags and settings after re-grabbing
		if (bHasGrabbable)
		{
			PlayerCharacter->bCanMove = true;
			GrabbedComponent->SetSimulatePhysics(true);
		}
	}
}

void UGrabber::RotateZ()
{
	if (bHasGrabbable)
	{
		// Stop player movement (Assuming APlayerCharacter has a function to stop movement)
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
		if (PlayerCharacter)
		{
			PlayerCharacter->bCanMove = false;
		}

		// Turn off physics in the grabbed component
		UPrimitiveComponent* GrabbedComponent = GetPhysicsHandle()->GetGrabbedComponent();
		if (GrabbedComponent)
		{
			GrabbedComponent->SetSimulatePhysics(false);
		}

		// Release from the physics handler
		GetPhysicsHandle()->ReleaseComponent();
		bHasGrabbable = false;

		// Rotate the grabbed component 90 degrees in the X-axis
		FRotator CurrentRotation = GrabbedComponent->GetComponentRotation();
		// Rotate the grabbed component by adding 90 degrees to the total rotation
		TotalRotation += 90.0f;

		// Ensure total rotation is within the range of 0 to 360 degrees
		TotalRotation = FMath::Fmod(TotalRotation, 360.0f);

		FRotator NewRotation = FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw , TotalRotation);
		GrabbedComponent->SetWorldRotation(NewRotation);

		// Grab the component again

		Grab();

		// Turn on necessary flags and settings after re-grabbing
		if (bHasGrabbable)
		{
			PlayerCharacter->bCanMove = true;
			GrabbedComponent->SetSimulatePhysics(true);
		}
	}
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



