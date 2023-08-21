// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AmmoType.h"
#include "PlayerCharacter.generated.h"


UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECState_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECState_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	ECState_Reloading UMETA(DisplayName = "Reloading"),

	ECSState_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FInterpLocation
{
	GENERATED_BODY()

	// Scene component for location interping
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SceneComponent;

	// Amount of items interping at the scene component location
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ItemCount;
};

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

	// Called for left and right input
	void MoveRight(float Value);

	// Turn at a given rate. This is a normalized rate, 1.0 = 100% of desired turn rate
	void TurnAtRate(float Rate);

	// look up/down at a given rate. This is a normalized rate, 1.0 = 100% of desired turn rate
	void LookUpAtRate(float Rate); 
		
	// Rotate controller based on mouse X movement
	// Value The input value from mouse movement
	void Turn(float Value);

	// Rotate controller based on mouse Y movement
	// param Value   The input value from mouse movement
	void LookUp(float Value);

	// Called when the Fire Button is pressed
	void FireWeapon();

	// Calculation for shotting stating and end location
	bool GetBeamEndLocations(const FVector& MuzzleSocketLocation_L, FHitResult& OutHitResult_L, const FVector& MuzzleSocketLocation_R, FHitResult& OutHitResult_R);

	// Set bAiming to true or false with button press
	void AimingButtonPressed();
	void AimingButtonReleased();

	void CameraInterpZoom(float DeltaTime);

	// Set BaseTurnRate and BaseLookUpRate based on aiming
	void SetLookRates();
	
	// Calculates the appropriate crosshair spread
	void CalculateCrosshairSpread(float DeltaTime);

	void StartCrosshairBulletFire();

	UFUNCTION()
	void FinishCrosshairBulletFire();
	
	void FireButtonPressed();
	void FireButtonReleased();
	void StartFireTimer();
	
	UFUNCTION()
	void AutoFireReset();

	// Line trace for detecting what we hit
	bool TraceUnderCrooshairs(FHitResult& OutHitResult, FVector& OutHitLocation);

	// trace items if OverlappedItemCount > 0
	void TraceForItems();

	// Spawns a default weapon on hand socket
	class AWeapon* SpawnDefaultWeapon();

	// Sttach weapon to player mesh 
	void EquipWeapon(AWeapon* WeaponToEquip);

	// Drop weapon
	void DropWeapon();

	void SelectButtonPressed();
	void SelectButtonReleased();

	// Swaps weapons 
	void SwapWeapon(AWeapon* WeaponToSwap);

	// Initialize Ammo Map with ammo values
	void InitializeAmmoMap();

	// Check if weapon has ammo
	bool WeaponHasAmmo();

	// FireWeapon functions
	void PlayFireSound();
	void SpawnBullet();
	void PlayGunfireAnim();

	// R key or gamepadFaceLeft button to Reload
	void ReloadButtonPressed();

	// Reload weapon
	void ReloadWeapon();

	// Checks to see if we have ammo of the EquippedWeapon's ammo type
	bool AmmoTypeCarried();

	// Called from Animation Blueprint with Grab Clip notify
	UFUNCTION(BlueprintCallable)
	void GrabClip();

	// Called from Animation Blueprint with Release Clip notify
	UFUNCTION(BlueprintCallable)
	void ReleaseClip();

	void PickupAmmo(class AAmmo* Ammo);

	void InitializeInterpLocations();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// Camera boom positioning the camera behind tge player 
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) // exposes a private variable to blueprint
	class USpringArmComponent* CameraBoom;
	// Follow Camera 
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) // exposes a private variable to blueprint
	class UCameraComponent* FollowCamera;

	// Base turn rate (turn sensitivity), in deg/sec. Other scaling may affect final turn rate
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	// Base Luck up/down rate (look sensitivity), in deg/sec. Other scaling may affect final turn rate
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float  BaseLookUpRate;

	// Turn rate while not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipTurnRate;

	// Look up rate when not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate;

	// Turn rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate;

	// Look up rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingLookUpRate;

	// Scale factor for mouse look sensitivity. Turn rate when not aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseHipTurnRate;

	// Scale factor for mouse look sensitivity. Look up rate when not aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseHipLookUpRate;
	 
	// Scale factor for mouse look sensitivity. Turn rate when aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseAimingTurnRate;

	// Scale factor for mouse look sensitivity. Look up rate when aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseAimingLookUpRate;

	// Randomizing shot sound 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class USoundCue* FireSound;

	// Flash spawned at BarrelSocket for both weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash_R;

	// firing the weapon montage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HipFireMontage;

	// Bullet impact Particles 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	// bullets smoke trail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;
	
	// True when aiming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	// Default camera field of view value
	float CameraDefaultFOV;

	// Aiming field of view
	float CameraZoomedFOV;

	// Current field of view this frame
	float CameraCurrentFOV;

	// Interpolate speed for zooming when aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed;

	// Spread of the crosshairs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier;

	// Speed component for crosshairs spread
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor;

	// In air component for crosshairs spread
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor;

	// Aim component for crosshairs spread
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor;

	// Shooting component for crosshairs spread
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor;

	// Line trace lenght
	float TraceLenght;

	// Line Trace lenght multiplier
	float TraceMultiplier;

	float ShootTimeDuration;
	bool bFiringBullet;
	FTimerHandle CrosshairShootTimer;

	// Left mouse button or contoller right trigger pressed
	bool bFireButtonPressed;

	// True when we can fire. False when waiting for the timer
	bool bShouldFire;

	// Rate of fire automatic gun
	float AutomaticFireRate;

	// Sets a timer between gunshots
	FTimerHandle AutoFireTimer;

	// True if we should trace every frame for items
	bool bShouldTraceForItems;

	// Number of overlapped AItems
	int8 OverlappedItemCount;

	// to store the AItem hitted last frame
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	class AItem* TraceHitItemLastFrame;

	// Current Weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon_R;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon_L;

	// Set default Weapon in Blueprints
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	// Current object hit by the trace
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AItem* TraceHitItem;

	// Forward distance from the camera, for the interpolation location
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance;

	// Up distance from the camera, for the interpolation location
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation;

	// Map to keep track of ammo of the different ammo types 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	// Default 9mm ammo amount
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo;

	// Default AR ammo amount
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmo;

	// Combat State, fire or reload if Unoccupied 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState;

	// Reload Anim montage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	// Transform of the clip when we first grab the clip when reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FTransform ClipTransform;

	// Scene component to attach to the character hand when reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* WeaponInterpComp;

	// Scene components to handle diferent interp animations for ammo pickups
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent4;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent5;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent6;

	// Array of interp location structs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FInterpLocation> InterpLocations;

public:
	// Returns CameraBoom (SpringArmComponent) subobject
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// Returns FollowCamera to subobject
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool GetAiming() const { return bAiming; }

	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplier() const;

	FORCEINLINE int8 GetOverlappedItemCount() const { return OverlappedItemCount; }

	// Adds or subtracts to/from OverlappedItemCount and updates bShouldTraceForItems
	void IncrementOverlappedItemCount(int8 Amount);

	//FVector GetCameraInterpLocation(); Item has getInterpLocation

	void GetPickupItem(AItem* Item);

	FInterpLocation GetInterpLocation(int32 Index);

	// Returns the index in InterpLocations array with the lowest ItemCount
	int32 GetInterpLocationindex();

	void IncrementInterpLocItemCount(int32 Index, int32 Amount);
};
