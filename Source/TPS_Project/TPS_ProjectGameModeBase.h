// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPS_ProjectGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TPS_PROJECT_API ATPS_ProjectGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	virtual void StartPlay() override;
	
};
