// Copyright Epic Games, Inc. All Rights Reserved.


#include "TPS_ProjectGameModeBase.h"

void ATPS_ProjectGameModeBase::StartPlay()
{
	Super::StartPlay();

	check(GEngine != nullptr);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("This is FPSGameMode!"));
};
