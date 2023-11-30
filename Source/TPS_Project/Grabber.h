// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Grabber.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPS_PROJECT_API UGrabber : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrabber();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called for grab input
	UFUNCTION(BlueprintCallable)
	void Grab();

	UFUNCTION(BlueprintCallable)
	void Released();

	// Called to Rotate the grabbed component
	UFUNCTION()
	void RotateX();
	UFUNCTION()
	void RotateZ();

private:

	UPROPERTY(EditAnywhere)
	float MaxGrabDistance;

	UPROPERTY(EditAnywhere)
	float GrabRadius = 100.0f;

	UPROPERTY(EditAnywhere)
	float HoldDistance = 500.0f;

	class UPhysicsHandleComponent* GetPhysicsHandle() const;

	bool GetGrabbableInreach(FHitResult& OutHitResult) const;
	bool bHasGrabbable;

	FRotator AccumulatedRotation;

	FRotator InitialRotation;

	float TotalRotation;

	UPROPERTY(EditAnywhere)
	float MouseRotationTurnRate;

		
public:

	FORCEINLINE bool GetHasGrabbable() const { return bHasGrabbable; }
};
