// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class TPS_PROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called for forward and back inputs
	void MoveForward(float Value);

	// Called for left and right movement
	void MoveRight(float Value);

	/**
	*Called viua input to turn at a given rate.
	* @param rate, this is a normalized rate, 1.0 = 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	*Called viua input to look up/down at a given rate.
	* @param rate, this is a normalized rate, 1.0 = 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate); 

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	/** Camera boom positioning the camera behind tge player */
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) // exposes a private variable to blueprint
	class USpringArmComponent* CameraBoom;
	/** Follow Camera */
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) // exposes a private variable to blueprint
	class UCameraComponent* FollowCamera;

	/** Base turn rate (turn sensitivity), in deg/sec. Other scaling may affect final turn rate */
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	/** Base Luck up/down rate (look sensitivity), in deg/sec. Other scaling may affect final turn rate */
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float  BaseLookUpRate;

public:
	/** Returns CameraBoom (SpringArmComponent) subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera to subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }


};
