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
	TraceLenght(50'000.0f)
	

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
	// To get the size of the viewport we need global GEngine variable
	FVector2D ViewPortSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize); // GetViewportSize with fill in with ViewPortSize with th ecurrent size of th eviewport
	}

	// Get CrossHair Location on screen
	FVector2D CrosshairLocation(ViewPortSize.X / 2.0f, ViewPortSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);

	if (bScreenToWorld) // if deprojection successful
	{
		// varibales for firing left weapon from crosshair position
		FHitResult ScreenTraceHit_L;
		const FVector Start_L{ CrosshairWorldPosition };
		const FVector End_L{ CrosshairWorldPosition + CrosshairWorldDirection * TraceLenght };

		// varibales for firing right weapon from crosshair position 
		FHitResult ScreenTraceHit_R;
		const FVector Start_R{ CrosshairWorldPosition };
		const FVector End_R{ CrosshairWorldPosition + CrosshairWorldDirection * TraceLenght };

		// To play the smoke particles until we hit something 
		//FVector BeamEndPoint_L{ End_L };
		OutBeamLocation_L = End_L;
		//FVector BeamEndPoint_R{ End_R };
		OutBeamLocation_R = End_R;

		// Setting Line tracing from crosshair to world position
		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit_L, Start_L, End_L, ECollisionChannel::ECC_Visibility);
		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit_R, Start_R, End_R, ECollisionChannel::ECC_Visibility);

		// If we hit something
		if (ScreenTraceHit_L.bBlockingHit && ScreenTraceHit_R.bBlockingHit)
		{
			// Beam end point is now trace hit location
			OutBeamLocation_L = ScreenTraceHit_L.Location;
			OutBeamLocation_R = ScreenTraceHit_R.Location;
		}

		//Perform a second trace, from the gun barrel to add precision when there is another object between the barrel line and crosshair
		FHitResult WeaponTraceHit_L; // Ray casting for firing left weapon
		const FVector WeaponTraceStart_L{ MuzzleSocketLocation_L };
		const FVector WeaponTraceEnd_L{ OutBeamLocation_L };

		FHitResult WeaponTraceHit_R; // Ray casting for firing right weapon
		const FVector WeaponTraceStart_R{ MuzzleSocketLocation_R };
		const FVector WeaponTraceEnd_R{ OutBeamLocation_R };

		GetWorld()->LineTraceSingleByChannel(WeaponTraceHit_L, WeaponTraceStart_L, WeaponTraceEnd_L, ECollisionChannel::ECC_Visibility);
		GetWorld()->LineTraceSingleByChannel(WeaponTraceHit_R, WeaponTraceStart_R, WeaponTraceEnd_R, ECollisionChannel::ECC_Visibility);

		if (WeaponTraceHit_L.bBlockingHit && WeaponTraceHit_R.bBlockingHit) // if there is an object between barrel and BeamEndPoints
		{
			OutBeamLocation_L = WeaponTraceHit_L.Location;
			OutBeamLocation_R = WeaponTraceHit_R.Location;
		}
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
	//  FMath::FInterpTo takes a target and  interSpeed, this are the magic numbers
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
bool APlayerCharacter::TraceUnderCrooshairs(FHitResult& OutHitResult)
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
		FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * TraceLenght };
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (OutHitResult.bBlockingHit)
		{
			return true;
		}
	}
	return false;
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

	// temp to test
	FHitResult ItemTraceResult;
	TraceUnderCrooshairs(ItemTraceResult);
	if (ItemTraceResult.bBlockingHit)
	{
		AItem* HitItem = Cast<AItem>(ItemTraceResult.GetActor());
		if (HitItem && HitItem->GetPickupWidget())
		{
			// Enable Item's Pickup Widget
			HitItem->GetPickupWidget()->SetVisibility(true);
		}
	}

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

}

float APlayerCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}
