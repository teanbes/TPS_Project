// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h" 

void UPlayerAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (PlayerCharacter == nullptr)
	{
		PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	}

	if (PlayerCharacter)
	{
		// Get the lateral speed of the player from velocity
		FVector Velocity{ PlayerCharacter->GetVelocity() };
		Velocity.Z = 0;
		Speed = Velocity.Size(); // magnitud of velocity

		// If player is in the air
		bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling(); // get characterMovement component to call isFalling

		// If the player is moving (accelerating)
		if (PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f) //  get characterMovement component to call magnitud of current acceleration
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		// calcualating Aim Base rotation
		FRotator AimRotation = PlayerCharacter->GetBaseAimRotation(); // returns rotator corresponding in the direction we are aiming
		FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);

		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(PlayerCharacter->GetVelocity()); // using Kismet library to create a rotator from a direction vector
		
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,AimRotation).Yaw; //  get the Normal between the move and the rotation for strafing animation
		if (PlayerCharacter->GetVelocity().Size() > 0.0f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}
		//FString OffSetMessage =FString::Printf(TEXT("Movement offset: %f"),MovementRotation.Yaw); 

		//FString MovementRotationMessage =FString::Printf(TEXT("Movement Rotation: %f"),MovementRotation.Yaw);

		//if (GEngine)
		//{
		//	GEngine->AddOnScreenDebugMessage(1, 0, FColor::White, OffSetMessage);
		//}
	}

}


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner()); // store playercharacter pawn owner
}


