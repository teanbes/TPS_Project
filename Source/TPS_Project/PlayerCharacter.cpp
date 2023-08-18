// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Item.h"
#include "Components/WidgetComponent.h"
#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"



// Sets default values
APlayerCharacter::APlayerCharacter() :
	// Rates for turning and looking up
	BaseTurnRate(45.0f),
	BaseLookUpRate(45.0f),
	// true when aiming the weapon
	bAiming(false),
	// Camera field of view values
	CameraDefaultFOV(0.0f), // setting in BeginPlay
	CameraZoomedFOV(40.0f),
	CameraCurrentFOV(0.0f),
	ZoomInterpSpeed(30.0f),
	// Turn rates for aiming
	HipTurnRate(90.0f),
	HipLookUpRate(90.0f),
	AimingTurnRate(20.0f),
	AimingLookUpRate(20.0f),
	// Mouse look sensitivity scale factors 
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	// Crosshair spread factors
	CrosshairSpreadMultiplier(0.0f),
	CrosshairVelocityFactor(0.0f),
	CrosshairInAirFactor(0.0f),
	CrosshairAimFactor(0.0f),
	CrosshairShootingFactor(0.0f),
	// Bullet fire timer variables
	ShootTimeDuration(0.05f),
	bFiringBullet(false),
	// Automatic fire variables
	AutomaticFireRate(0.1f),
	bShouldFire(true),
	bFireButtonPressed(false),
	// Line tracing variables
	TraceLenght(50'000.0f),
	TraceMultiplier(1.25f),
	bShouldTraceForItems(false),
	OverlappedItemCount(0),
	// Camera interpolation location variables
	CameraInterpDistance(250.0f),
	CameraInterpElevation(65.0f),
	// Starting ammo amounts
	Starting9mmAmmo(85),
	StartingARAmmo(120)
	

{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Creating USpringArmComponent and asigning to the CameraBoom (pulls in towards the player if there is collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 270.0f; // The camera follows teh player at this distance
	CameraBoom->bUsePawnControlRotation = true; // controls rotation
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 70.0f); // Offset the camera for the crosshair

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm, just follow the cameraboom rotation

	// 
	// Don't rotate player when the controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; // to rotate towards mouse movement in the yaw
	bUseControllerRotationRoll = false;

	// Player movement independent of camera movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // player moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(LogTemp, Warning, TEXT("BeginPlay() called!"));

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
	// Spawn default weapons and attach and equip
	EquipWeapon(SpawnDefaultWeapon());

	InitializeAmmoMap();
}

void APlayerCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// Find which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) }; // Gets axis direction from rotation matrix 
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// Find which way is right
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) }; // Gets axis direction from rotation matrix 
		AddMovementInput(Direction, Value);
	}
}

// Controlls yaw
void APlayerCharacter::TurnAtRate(float Rate)
{
	// Delta for this frame from the rate data
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds()); //  deg/sec
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); //  deg/sec
}

void APlayerCharacter::Turn(float Value)
{
	float TurnScaleFactor{};
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value * TurnScaleFactor);

}

void APlayerCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpScaleFactor);
}

void APlayerCharacter::FireWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("Firing Weapon"));
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	// Getting the player mesh to get the socket to spawn the particles
	const USkeletalMeshSocket* BarrelSocket_L = GetMesh()->GetSocketByName("BarrelSocket_L");
	const USkeletalMeshSocket* BarrelSocket_R = GetMesh()->GetSocketByName("BarrelSocket_R");

	if (BarrelSocket_L && BarrelSocket_R)
	{
		const FTransform SocketTransform_L = BarrelSocket_L->GetSocketTransform(GetMesh());// getting transform of the new socket
		const FTransform SocketTransform_R = BarrelSocket_R->GetSocketTransform(GetMesh());// getting transform of the new socket

		// if MuzzleFlash, spawn the particle at socket transform and rotation
		if (MuzzleFlash_L && MuzzleFlash_R)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash_L, SocketTransform_L);
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash_R, SocketTransform_R);
		}

		// Funcion to get info for spaining impacts and FX
		FVector BeamEnd_L;
		FVector BeamEnd_R;
		bool bBeamsEnd = GetBeamEndLocations(SocketTransform_L.GetLocation(), BeamEnd_L, SocketTransform_R.GetLocation(), BeamEnd_R);

		if (bBeamsEnd)
		{
			// spawn impact particles after updating BeamEndPoints
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd_L);
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd_R);
			}

			if (BeamParticles)
			{
				UParticleSystemComponent* Beam_L = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_L);
				UParticleSystemComponent* Beam_R = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_R);
				if (Beam_L && Beam_R)
				{
					Beam_L->SetVectorParameter(FName("Target"), BeamEnd_L);
					Beam_R->SetVectorParameter(FName("Target"), BeamEnd_R);
				}
			}
		}
	}

	// Play shooting animation
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
	//Start bullet fire timer for crosshairs
	StartCrosshairBulletFire();

}

bool APlayerCharacter::GetBeamEndLocations(const FVector& MuzzleSocketLocation_L, FVector& OutBeamLocation_L, const FVector& MuzzleSocketLocation_R, FVector& OutBeamLocation_R)
{
	// Check for crosshair trace hit left gun
	FHitResult CrosshairHitResult_L;
	bool bCrosshairHit_L = TraceUnderCrooshairs(CrosshairHitResult_L, OutBeamLocation_L);
	// Check for crosshair trace hit right gun
	FHitResult CrosshairHitResult_R;
	bool bCrosshairHit_R = TraceUnderCrooshairs(CrosshairHitResult_R, OutBeamLocation_R);

	if (bCrosshairHit_L && bCrosshairHit_R)
	{
		// Tentative beam location - still need to trace from gun
		OutBeamLocation_L = CrosshairHitResult_L.Location;
		OutBeamLocation_R = CrosshairHitResult_R.Location;

	}
	else // no crosshair trace hit
	{
		// OutBeamLocation is the End location for the line trace
	}

	//Perform a second trace, from the gun barrel to add precision when there is another object between the barrel line and crosshair
	FHitResult WeaponTraceHit_L; // Ray casting for firing left weapon
	const FVector WeaponTraceStart_L{ MuzzleSocketLocation_L };
	const FVector StartToEnd_L{ OutBeamLocation_L - MuzzleSocketLocation_L };
	const FVector WeaponTraceEnd_L{ MuzzleSocketLocation_L + StartToEnd_L * TraceMultiplier};

	FHitResult WeaponTraceHit_R; // Ray casting for firing right weapon
	const FVector WeaponTraceStart_R{ MuzzleSocketLocation_R };
	const FVector StartToEnd_R{ OutBeamLocation_R - MuzzleSocketLocation_R };
	const FVector WeaponTraceEnd_R{ MuzzleSocketLocation_R + StartToEnd_R * TraceMultiplier };

	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit_L, WeaponTraceStart_L, WeaponTraceEnd_L, ECollisionChannel::ECC_Visibility);
	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit_R, WeaponTraceStart_R, WeaponTraceEnd_R, ECollisionChannel::ECC_Visibility);

	if (WeaponTraceHit_L.bBlockingHit && WeaponTraceHit_R.bBlockingHit) // if there is an object between barrel and BeamEndPoints
	{
		OutBeamLocation_L = WeaponTraceHit_L.Location;
		OutBeamLocation_R = WeaponTraceHit_R.Location;
		return true;
	}
	return false;
	
}

void APlayerCharacter::AimingButtonPressed()
{
	bAiming = true;
}

void APlayerCharacter::AimingButtonReleased()
{
	bAiming = false;
}

void APlayerCharacter::CameraInterpZoom(float DeltaTime)
{
	// If Aiming, Smooth camera field of view
	if (bAiming)
	{
		// Smooth to zoomed
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		// Smooth to unzoomed 
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);

}

void APlayerCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void APlayerCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.0f, 600.0f };
	FVector2D VelocityMultiplierRange{ 0.0f, 1.0f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.0f;

	// Calculate crosshair velocity factor
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	// If player is in the air, calculate crosshair in air factor
	//  FMath::FInterpTo takes a target and  interSpeed, these are the magic numbers
	if (GetCharacterMovement()->IsFalling()) 
	{
		// Spread the crosshairs slowly while in air
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else // Character is on the ground 
	{
		// Shrink the crosshairs rapidly while on the ground
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0f, DeltaTime, 30.0f);
	}

	// Calculate crosshair aim factor
	if (bAiming) // Are we aiming?
	{
		// Shrink crosshairs a small amount very quickly
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.0f);
	}
	else // Not aiming
	{
		// Spread crosshairs back to normal very quickly
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.0f);
	}

	// True 0.05 second after firing
	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.0f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 60.0f);
	}

	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
}

void APlayerCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &APlayerCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void APlayerCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void APlayerCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	StartFireTimer();
}

void APlayerCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void APlayerCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &APlayerCharacter::AutoFireReset, AutomaticFireRate);
	}

}

void APlayerCharacter::AutoFireReset()
{
	bShouldFire = true;
	// if we still pressing fire StartFireTimer() again
	if (bFireButtonPressed)
	{
		StartFireTimer();
	}
}

// trace to get objects in the world
bool APlayerCharacter::TraceUnderCrooshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// To get the size of the viewport we need global GEngine variable
	FVector2D ViewPortSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize); // GetViewportSize will fill in with ViewPortSize with the current size of the viewport
	}

	// Get CrossHair Location on screen
	FVector2D CrosshairLocation(ViewPortSize.X / 2.0f, ViewPortSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		// Trace from crosshair to world
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * TraceLenght };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}
	return false;
}

void APlayerCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrooshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				// Enable Item's Pickup Widget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
			}
			// We hit an AItem last frame
			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					//if we are not hitting the Aitem, disable item visivility
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}
			}
			// Store HitItem for next frame
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		// If player not overlaping sphere, disable item visivility
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
	}
}


AWeapon* APlayerCharacter::SpawnDefaultWeapon()
{
	// Check DefaultWeaponClass (TSubclassOf)
	if (DefaultWeaponClass)
	{
		// Spawn Weapon
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void APlayerCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip)
	{
		// Get the Hand Socket
		const USkeletalMeshSocket* HandSocket_R = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		const USkeletalMeshSocket* HandSocket_L = GetMesh()->GetSocketByName(FName("LeftHandSocket"));
		if (HandSocket_R && HandSocket_L)
		{
			// Spawn an instance of the weapon for each hand
			AWeapon* Weapon_R = WeaponToEquip;
			AWeapon* Weapon_L = GetWorld()->SpawnActor<AWeapon>(WeaponToEquip->GetClass());

			if (Weapon_R && Weapon_L)
			{
				//// Attach the Weapon to the hand socket RightHandSocket
				HandSocket_R->AttachActor(Weapon_R, GetMesh());
				HandSocket_L->AttachActor(Weapon_L, GetMesh());

				// Set EquippedWeapon to the spawned Weapon
				EquippedWeapon_R = Weapon_R;
				EquippedWeapon_L = Weapon_L;
				// Set the item state for each weapon
				EquippedWeapon_R->SetItemState(EItemState::EIS_Equipped);
				EquippedWeapon_L->SetItemState(EItemState::EIS_Equipped);
			}
		}
	}
}

// Throw weapon
void APlayerCharacter::DropWeapon()
{
	FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
	EquippedWeapon_R->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

	EquippedWeapon_R->SetItemState(EItemState::EIS_Falling);
	EquippedWeapon_R->ThrowWeapon();

	EquippedWeapon_L->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
	EquippedWeapon_L->Destroy();


}

void APlayerCharacter::SelectButtonPressed()
{
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
	}
}

void APlayerCharacter::SelectButtonReleased()
{

}

void APlayerCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	DropWeapon();
	EquipWeapon(WeaponToSwap);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void APlayerCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle zoom smoothing transition
	CameraInterpZoom(DeltaTime);

	// Handles look sensitivite if aiming of not
	SetLookRates();

	// Calculate crosshair spread multiplier
	CalculateCrosshairSpread(DeltaTime);

	// Check Over lapped Items, then trace 
	TraceForItems();

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &APlayerCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APlayerCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &APlayerCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &APlayerCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &APlayerCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &APlayerCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APlayerCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &APlayerCharacter::SelectButtonReleased);

}

float APlayerCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void APlayerCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

FVector APlayerCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation{ FollowCamera->GetComponentLocation() };
	const FVector CameraForward{ FollowCamera->GetForwardVector() };

	// LocationToInterpTo = CameraWorldLocation + ForwardLocation * A + UpVector * B
	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.0f, 0.0f, CameraInterpElevation);
}

// 
void APlayerCharacter::GetPickupItem(AItem* Item)
{
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		SwapWeapon(Weapon);
	}
}
