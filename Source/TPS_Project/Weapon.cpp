// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

AWeapon::AWeapon() :
	ThrowWeaponTime(0.7f),
	bFalling(false)
{
	PrimaryActorTick.bCanEverTick = true; // to ensure tick function is called
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// don't roll weapon
	if (GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation{ 0.0f, GetItemMesh()->GetComponentRotation().Yaw, 0.0f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation{ 0.0f, GetItemMesh()->GetComponentRotation().Yaw, 0.0f };
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward{ GetItemMesh()->GetForwardVector() };
	const FVector MeshRight{ GetItemMesh()->GetRightVector() };
	// Throw the Weapon direction
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-10.0f, MeshForward);

	float RandomRotation{ FMath::FRandRange(20.0f, 35.0f) }; 
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.0f, 0.0f, 1.0f));
	ImpulseDirection *= 2000.0f;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling,ThrowWeaponTime);

}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
}
