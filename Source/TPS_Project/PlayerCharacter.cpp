// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
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
#include "Ammo.h"
#include "BulletHitInterface.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Grabber.h"


// Sets default values
APlayerCharacter::APlayerCharacter() :
	// Rates for turning and looking up
	BaseTurnRate(45.0f),
	BaseLookUpRate(45.0f),
	// true when aiming the weapon
	bAiming(false),
	// Camera field of view values
	CameraDefaultFOV(0.0f), // setting in BeginPlay
	CameraZoomedFOV(30.0f),
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
	AutomaticFireRate(0.2f),
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
	StartingARAmmo(120),
	//Player Health
	Health(100.f),
	MaxHealth(100.f),
	bIsDeath(false),
	bIsDeadEye(false)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Creating USpringArmComponent and asigning to the CameraBoom (pulls in towards the player if there is collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 270.0f; // The camera follows teh player at this distance
	CameraBoom->bUsePawnControlRotation = true; // controls rotation
	CameraBoom->SocketOffset = FVector(0.0f, 70.0f, 70.0f); // Offset the camera for the crosshair

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

	// Create Hand Scene Component 
	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	// Create Interp Components
	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	InterpComponent1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComponent1->SetupAttachment(GetFollowCamera());

	InterpComponent2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComponent2->SetupAttachment(GetFollowCamera());

	InterpComponent3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComponent3->SetupAttachment(GetFollowCamera());

	InterpComponent4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComponent4->SetupAttachment(GetFollowCamera());

	InterpComponent5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComponent5->SetupAttachment(GetFollowCamera());

	InterpComponent6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComponent6->SetupAttachment(GetFollowCamera());

	GrabberRef = CreateDefaultSubobject<UGrabber>(TEXT("Grabber Component"));
	GrabberRef->SetupAttachment(GetFollowCamera());
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.0f)
	{
		Health = 0.0f;
		Die();

		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("PlayerDead")), true);
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void APlayerCharacter::Die()
{
	if (bIsDeath) return;

	bIsDeath = true;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void APlayerCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		DisableInput(PC);
	}
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

	// Create FInterpLocation structs for each interp location and add to array
	InitializeInterpLocations();
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
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds()); //  deg/frame
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); //  deg/frame
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
	if (EquippedWeapon_R == nullptr && EquippedWeapon_L == nullptr) return;
	if (CombatState != ECombatState::ECState_Unoccupied) return;

	if (WeaponHasAmmo())
	{
		PlayFireSound();
		SpawnBullet();
		PlayGunfireAnim();
		// Decrease ammo count
		EquippedWeapon_R->DecreaseAmmo();
		//EquippedWeapon_L->DecreaseAmmo(); one weapon for testing
		StartFireTimer();
	}
}

// Dead Eye****************************************************************
void APlayerCharacter::PerformDeadEye()
{
	if (!bIsDeadEye)
	{
		CombatState = ECombatState::ECState_DeadEye;
		// Slow down time
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1f);
		bIsDeadEye = true;
		DeadEyeWidget();
	}
	else
	{
		CombatState = ECombatState::ECState_Unoccupied;
		// Restore normal time
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
		bIsDeadEye = false;
	}
}

void APlayerCharacter::GravityPowerUp()
{
	UE_LOG(LogTemp, Display, TEXT("GravityPowerUp"));

	
	if (GrabberRef != nullptr)
	{
		GrabberRef->Grab();
	}
	else 
	{
		UE_LOG(LogTemp, Display, TEXT("There is no Grabber Component"));
	}
}

bool APlayerCharacter::GetBeamEndLocations(const FVector& MuzzleSocketLocation_L, FHitResult& OutHitResult_L, const FVector& MuzzleSocketLocation_R, FHitResult& OutHitResult_R)
{
	FVector OutBeamLocation_L;
	FVector OutBeamLocation_R;
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
	
	const FVector WeaponTraceStart_L{ MuzzleSocketLocation_L };
	const FVector StartToEnd_L{ OutBeamLocation_L - MuzzleSocketLocation_L };
	const FVector WeaponTraceEnd_L{ MuzzleSocketLocation_L + StartToEnd_L * TraceMultiplier};

	const FVector WeaponTraceStart_R{ MuzzleSocketLocation_R };
	const FVector StartToEnd_R{ OutBeamLocation_R - MuzzleSocketLocation_R };
	const FVector WeaponTraceEnd_R{ MuzzleSocketLocation_R + StartToEnd_R * TraceMultiplier };

	GetWorld()->LineTraceSingleByChannel(OutHitResult_L, WeaponTraceStart_L, WeaponTraceEnd_L, ECollisionChannel::ECC_Visibility);
	GetWorld()->LineTraceSingleByChannel(OutHitResult_R, WeaponTraceStart_R, WeaponTraceEnd_R, ECollisionChannel::ECC_Visibility);

	if (!OutHitResult_L.bBlockingHit && !OutHitResult_R.bBlockingHit) // if there is an object between barrel and BeamEndPoints
	{
		OutHitResult_L.Location = OutBeamLocation_L;
		OutHitResult_R.Location = OutBeamLocation_R;
		return false;
	}
	return true;
}

void APlayerCharacter::AimingButtonPressed()
{
	if (CombatState != ECombatState::ECState_Reloading)
	{
		bAiming = true;
	}
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

	FireWeapon();

}

void APlayerCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void APlayerCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		CombatState = ECombatState::ECState_FireTimerInProgress;

		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &APlayerCharacter::AutoFireReset, AutomaticFireRate);
	}
}

void APlayerCharacter::AutoFireReset()
{
	CombatState = ECombatState::ECState_Unoccupied;

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed)
		{
			FireWeapon();
		}
	}
	else
	{
		// Reload Weapon
		ReloadWeapon();
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

bool APlayerCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon_R == nullptr && EquippedWeapon_L == nullptr) return false;

	return EquippedWeapon_R->GetAmmo() > 0; // just one weapon for testing (&& EquippedWeapon_L->GetAmmo() > 0);
}

void APlayerCharacter::PlayFireSound()
{
	// Play fire sound
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void APlayerCharacter::SpawnBullet()
{
	// SPAWN BULLET
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
		FHitResult BeamHitResult_L;
		FHitResult BeamHitResult_R;

		bool bBeamsEnd = GetBeamEndLocations(SocketTransform_L.GetLocation(), BeamHitResult_L, SocketTransform_R.GetLocation(), BeamHitResult_R);

		if (bBeamsEnd)
		{
			// Does hit Actor implement BulletHitInterface?
			if (BeamHitResult_L.GetActor() && BeamHitResult_R.GetActor())
			{
				IBulletHitInterface* BulletHitInterface_L = Cast<IBulletHitInterface>(BeamHitResult_L.GetActor());
				IBulletHitInterface* BulletHitInterface_R = Cast<IBulletHitInterface>(BeamHitResult_R.GetActor());

				if (BulletHitInterface_L && BulletHitInterface_R)
				{
					BulletHitInterface_L->BulletHit_Implementation(BeamHitResult_L, this, GetController());
					BulletHitInterface_R->BulletHit_Implementation(BeamHitResult_R, this, GetController());
				}

				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult_R.GetActor());
				if (HitEnemy)
				{
					int32 Damage{};
					if (BeamHitResult_R.BoneName.ToString() == HitEnemy->GetHeadBone())
					{
						// Head shot
						Damage = EquippedWeapon_R->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(BeamHitResult_R.GetActor(), Damage, GetController(), this, UDamageType::StaticClass()); // Only Applying Damage Wth one weapon
						
						// HUD hit number
						HitEnemy->ShowHitNumber(Damage, BeamHitResult_R.Location, true );
					}
					else
					{
						// shot damage
						Damage = EquippedWeapon_R->GetDamage();
						UGameplayStatics::ApplyDamage(BeamHitResult_R.GetActor(), Damage, GetController(), this, UDamageType::StaticClass());// Only Applying Damage Wth one weapon

						// HUD hit number
						HitEnemy->ShowHitNumber(Damage, BeamHitResult_R.Location, false);
					}
					
				}
			}
			else
			{
				// spawn default impact particles after updating BeamEndPoints
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamHitResult_L.Location);
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamHitResult_R.Location);
				}

			}

			if (BeamParticles)
			{
				UParticleSystemComponent* Beam_L = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_L);
				UParticleSystemComponent* Beam_R = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_R);
				if (Beam_L && Beam_R)
				{
					Beam_L->SetVectorParameter(FName("Target"), BeamHitResult_L.Location);
					Beam_R->SetVectorParameter(FName("Target"), BeamHitResult_R.Location);
				}
			}
			
		}
	}
}

void APlayerCharacter::PlayGunfireAnim()
{
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

void APlayerCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void APlayerCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECState_Unoccupied) return;
	if (EquippedWeapon_R == nullptr) return;

	// Check if we have the right ammo type
	if (AmmoTypeCarried())
	{
		CombatState = ECombatState::ECState_Reloading; //  Reloading state

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();// Anim Instance
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon_R->GetReloadMontageSection());
		}
	}


}

bool APlayerCharacter::AmmoTypeCarried()
{
	if (EquippedWeapon_R == nullptr) return false;

	auto AmmoType = EquippedWeapon_R->GetAmmoType();

	// Check if map containts type
	if (AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

// to use in weapons that have clips to add realism to the animation ****** NOt USED YET***
void APlayerCharacter::GrabClip()
{
	if (EquippedWeapon_R == nullptr) return;
	if (HandSceneComponent == nullptr) return;
	
	// Index for the clip bone on the Equipped Weapon
	int32 ClipBoneIndex{ EquippedWeapon_R->GetItemMesh()->GetBoneIndex(EquippedWeapon_R->GetClipBoneName()) };

	// Store the clip transform
	ClipTransform = EquippedWeapon_R->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true); // to atach the clip to the hand and keep the relative rotation
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L"))); // attaching the clkip to the Left hand
	HandSceneComponent->SetWorldTransform(ClipTransform); 

	EquippedWeapon_R->SetMovingClip(true);
}

void APlayerCharacter::ReleaseClip()
{

}

void APlayerCharacter::PickupAmmo(AAmmo* Ammo)
{
	// check if AmmoMap has the picking ammo type
	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		// Get ammo amount in the AmmoMap for this type
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();

		// Set ammo amount in the map for this type
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}


	if (EquippedWeapon_R->GetAmmoType() == Ammo->GetAmmoType())
	{
		// If gun is empty
		if (EquippedWeapon_R->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}

	Ammo->Destroy();
}

void APlayerCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComponent1, 0 };
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2{ InterpComponent2, 0 };
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3{ InterpComponent3, 0 };
	InterpLocations.Add(InterpLoc3);

	FInterpLocation InterpLoc4{ InterpComponent4, 0 };
	InterpLocations.Add(InterpLoc4);
	 
	FInterpLocation InterpLoc5{ InterpComponent5, 0 };
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6{ InterpComponent6, 0 };
	InterpLocations.Add(InterpLoc6);
}


int32 APlayerCharacter::GetInterpLocationindex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for (int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if (InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}

	return LowestIndex;
}

void APlayerCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount > 1) return;

	if (InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
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

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &APlayerCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("DeadEye", IE_Pressed, this, &APlayerCharacter::PerformDeadEye);

	PlayerInputComponent->BindAction("Grab", IE_Pressed, this, &APlayerCharacter::GravityPowerUp);
}

void APlayerCharacter::FinishReloading()
{
	// Update States
	CombatState = ECombatState::ECState_Unoccupied;

	if (EquippedWeapon_R == nullptr) return;

	const auto AmmoType{ EquippedWeapon_R->GetAmmoType() };

	// Update ammo map
	if (AmmoMap.Contains(AmmoType))
	{
		// Ammo Amount carried per weapon type
		int32 CarriedAmmo = AmmoMap[AmmoType];

		// Space left in the magazine of EquippedWeapon
		const int32 MagEmptySpace = EquippedWeapon_R->GetMagazineCapacity() - EquippedWeapon_R->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			// Reload magazine with rest of ammo carried
			EquippedWeapon_R->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo); // update map
		}
		else
		{
			// fill the magazine
			EquippedWeapon_R->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo); // update map
		}

	}
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

/* Item has getInterpLocation
FVector APlayerCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation{ FollowCamera->GetComponentLocation() };
	const FVector CameraForward{ FollowCamera->GetForwardVector() };

	// LocationToInterpTo = CameraWorldLocation + ForwardLocation * A + UpVector * B
	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.0f, 0.0f, CameraInterpElevation);
}
*/

// 
void APlayerCharacter::GetPickupItem(AItem* Item)
{
	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}

	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		SwapWeapon(Weapon);
	}

	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo)
	{
		PickupAmmo(Ammo);
	}
}

FInterpLocation APlayerCharacter::GetInterpLocation(int32 Index)
{
	if (Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}
