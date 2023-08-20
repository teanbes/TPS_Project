// Fill out your copyright notice in the Description page of Project Settings.


#include "Ammo.h"

AAmmo::AAmmo()
{
	AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoMesh"));
	SetRootComponent(AmmoMesh);
}
