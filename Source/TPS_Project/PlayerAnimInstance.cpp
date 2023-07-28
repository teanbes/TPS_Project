// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	}

}


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner()); // store playercharacter pawn owner
}


