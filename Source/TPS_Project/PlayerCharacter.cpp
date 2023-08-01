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


// Sets default values
APlayerCharacter::APlayerCharacter() : BaseTurnRate(45.f), BaseLookUpRate(45.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Creating USpringArmComponent and asigning to the CameraBoom (pulls in towards the player if there is collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f; // The camera follows teh player at this distance
	CameraBoom->bUsePawnControlRotation = true; // controls rotation
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f); // Offset the camera for the crosshair

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
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("BeginPlay() called!"));
	
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

		

		

		/*  ----This code traces the gun shots from the gun barrel socket----
		// Ray casting for firing left weapon
		FHitResult FireHit_L;
		const FVector Start_L{ SocketTransform_L.GetLocation() };
		const FQuat Rotation_L { SocketTransform_L.GetRotation() };
		const FVector RotationAxix_L{ Rotation_L.GetAxisX() };
		const FVector End_L{ Start_L + RotationAxix_L * 50000.f };

		// Ray casting for firing Right weapon
		FHitResult FireHit_R;
		const FVector Start_R{ SocketTransform_R.GetLocation() };
		const FQuat Rotation_R{ SocketTransform_R.GetRotation() };
		const FVector RotationAxix_R{ Rotation_R.GetAxisX() };
		const FVector End_R{ Start_R + RotationAxix_R * 50000.f };

		// To play the smoke particles until we hit something
		FVector BeamEndPoint_L{ End_L }; 
		FVector BeamEndPoint_R{ End_R };


		GetWorld()->LineTraceSingleByChannel(FireHit_L, Start_L, End_L, ECollisionChannel::ECC_Visibility);
		GetWorld()->LineTraceSingleByChannel(FireHit_R, Start_R, End_R, ECollisionChannel::ECC_Visibility);
		if (FireHit_L.bBlockingHit && FireHit_R.bBlockingHit)
		{
			//DrawDebugLine(GetWorld(), Start_L, End_L, FColor::Red, false, 2.0f);
			//DrawDebugLine(GetWorld(), Start_R, End_R, FColor::Red, false, 2.0f);
			//
			//DrawDebugPoint(GetWorld(), FireHit_L.Location, 5.0f, FColor::Red, false, 2.0f);
			//DrawDebugPoint(GetWorld(), FireHit_R.Location, 5.0f, FColor::Red, false, 2.0f);

			BeamEndPoint_L = FireHit_L.Location;
			BeamEndPoint_R = FireHit_R.Location;

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit_L.Location);
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit_R.Location);
			}

		}

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam_L = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_L);
			UParticleSystemComponent* Beam_R = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform_R);
			if (Beam_L && Beam_R)
			{
				Beam_L->SetVectorParameter(FName("Target"), BeamEndPoint_L);
				Beam_R->SetVectorParameter(FName("Target"), BeamEndPoint_R);
			}
		}
		*/
	}

	// Play shooting animation
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}

}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump",IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &APlayerCharacter::FireWeapon);


}

bool APlayerCharacter::GetBeamEndLocations(const FVector& MuzzleSocketLocation_L, FVector& OutBeamLocation_L, const FVector& MuzzleSocketLocation_R, FVector& OutBeamLocation_R)
{
	//To get the size of th eviewport we need global GEngine variable
	FVector2D ViewPortSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize); // GetViewportSize with fill in with ViewPortSize with th ecurrent size of th eviewport
	}

	// Get CrossHair Location on screen
	FVector2D CrosshairLocation(ViewPortSize.X / 2.0f, ViewPortSize.Y / 2.0f);
	CrosshairLocation.Y -= 35.0f; // Offseting Crosshair location
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);

	if (bScreenToWorld) // if deprojection successful
	{
		// varibales for firing left weapon from crosshair position
		FHitResult ScreenTraceHit_L;
		const FVector Start_L{ CrosshairWorldPosition };
		const FVector End_L{ CrosshairWorldPosition + CrosshairWorldDirection * 50000.f };

		// varibales for firing right weapon from crosshair position 
		FHitResult ScreenTraceHit_R;
		const FVector Start_R{ CrosshairWorldPosition };
		const FVector End_R{ CrosshairWorldPosition + CrosshairWorldDirection * 50000.f };

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


