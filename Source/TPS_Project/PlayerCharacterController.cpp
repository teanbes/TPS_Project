// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterController.h"
#include "Blueprint/UserWidget.h"

APlayerCharacterController::APlayerCharacterController()
{

}

void APlayerCharacterController::BeginPlay()
{
	Super::BeginPlay();

	// Check our HUDOverlayClass TSubclassOf variable
	if (HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);// creating an instance of HUDOverlayClass
		if (HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}