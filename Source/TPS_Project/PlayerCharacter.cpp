// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"


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

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm, just follow the cameraboom rotation

	// 
	// Don't rotate player when the controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Player movement independent of camera movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // player moves in the direction of input...
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

	const USkeletalMeshSocket* BarrelSocket_L = GetMesh()->GetSocketByName("BarrelSocket_L");// Getting the player mesh to get the socket to spawn the particles
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

		GetWorld()->LineTraceSingleByChannel(FireHit_L, Start_L, End_L, ECollisionChannel::ECC_Visibility);
		GetWorld()->LineTraceSingleByChannel(FireHit_R, Start_R, End_R, ECollisionChannel::ECC_Visibility);
		if (FireHit_L.bBlockingHit && FireHit_R.bBlockingHit)
		{
			DrawDebugLine(GetWorld(), Start_L, End_L, FColor::Red, false, 2.0f);
			DrawDebugLine(GetWorld(), Start_R, End_R, FColor::Red, false, 2.0f);

			DrawDebugPoint(GetWorld(), FireHit_L.Location, 5.0f, FColor::Red, false, 2.0f);
			DrawDebugPoint(GetWorld(), FireHit_R.Location, 5.0f, FColor::Red, false, 2.0f);

		}


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

