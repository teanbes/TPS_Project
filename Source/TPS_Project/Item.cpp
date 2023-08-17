// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"

// Sets default values
AItem::AItem(): 
	ItemName(FString("Default")),
	ItemCount(0), 
	ItemRarity(EItemRarity::EIR_Common), 
	ItemState(EItemState::EIS_Pickup),
	// Item interping variables
	ZCurveTime(0.7f),
	ItemInterpStartLocation(FVector(0.0f)),
	CameraTargetLocation(FVector(0.0f)),
	bInterping(false),
	ItemInterpX(0.0f),
	ItemInterpY(0.0f),
	InterpInitialYawOffset(0.0f)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	// Setting collision box to be hit by the line trace
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // Ignoring all collision chanell
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,ECollisionResponse::ECR_Block); // setting the channel need it
	
	// Creating widget component
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();

	// Hide Pickup widget
	if (PickupWidget)
		PickupWidget->SetVisibility(false);

	// Sets ActiveStars based on rarity 
	SetActiveStars();


	// Setup overlap for AreaSphere
	// // AddDynamic binds a funtion to the OnComponentBeginOverlap, takes a function address and store it to call it when somiething overlaps with AreaSphere the function gets called
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap); 
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);
	
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
		if (PlayerCharacter)
		{
			PlayerCharacter->IncrementOverlappedItemCount(1);
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
		if (PlayerCharacter)
		{
			PlayerCharacter->IncrementOverlappedItemCount(-1);
		}
	}
}

void AItem::SetActiveStars()
{
	for (int i = 0; i <= 5; i++)
	{
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
		case EItemRarity::EIR_Damaged:
			ActiveStars[1] = true;
			break;
		case EItemRarity::EIR_Common:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			break;
		case EItemRarity::EIR_Uncommon:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			break;
		case EItemRarity::EIR_Rare:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			break;
		case EItemRarity::EIR_Legendary:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			ActiveStars[5] = true;
			break;
	}
}

void AItem::SetItemProperties(EItemState State)
{
	switch (State)
	{
		case EItemState::EIS_Pickup:
			// Mesh properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// AreaSphere properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			// CollisionBox properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(
				ECollisionChannel::ECC_Visibility,
				ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			break;

		case EItemState::EIS_Equipped:
			PickupWidget->SetVisibility(false);
			// Mesh properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// AreaSphere properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// CollisionBox properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;

		case EItemState::EIS_Falling:
			// Mesh properties
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetEnableGravity(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			// AreaSphere properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// CollisionBox properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;

		case EItemState::EIS_EquipInterping:
			PickupWidget->SetVisibility(false);
			// Mesh properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// AreaSphere properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// CollisionBox properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;

	}
}

void AItem::FinishInterping()
{
	bInterping = false;
	if (Character)
	{
		Character->GetPickupItem(this);
	}
	// scale back to normal
	SetActorScale3D(FVector(1.0f));
}

void AItem::ItemInterp(float DeltaTime)
{
	if (!bInterping) return;

	if (Character && ItemZCurve)
	{
		// Elapsed time since we started ItemInterpTimer
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		// Get curve value corresponding to ElapsedTime
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);
		
		// Get the item's initial location when the curve started
		FVector ItemLocation = ItemInterpStartLocation;
		// Get location in front of the camera
		const FVector CameraInterpLocation{ Character->GetCameraInterpLocation() };

		// Vector from Item to Camera Interp Location, X and Y are zeroed out
		const FVector ItemToCamera{ FVector(0.0f, 0.0f, (CameraInterpLocation - ItemLocation).Z) };
		// Scale factor to multiply with CurveValue
		const float DeltaZ = ItemToCamera.Size();
		// Interping Speed
		const float InterpSpeed = 30.0f;
		const FVector CurrentLocation{ GetActorLocation() };
		const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, InterpSpeed);
		// Interpolated Y value
		const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y,	CameraInterpLocation.Y, DeltaTime, InterpSpeed);

		// Set X and Y of ItemLocation to Interped values
		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;

		// Adding curve value to the Z component of the Initial Location (scaled by DeltaZ)
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

		// Camera rotation this frame
		const FRotator CameraRotation{ Character->GetFollowCamera()->GetComponentRotation() };
		// Camera rotation plus inital Yaw Offset
		FRotator ItemRotation{ 0.0f, CameraRotation.Yaw + InterpInitialYawOffset, 0.0f };
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

		if (ItemScaleCurve)
		{
			const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
		}
	}

}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle item interping when in EquipInterping state
	ItemInterp(DeltaTime);
}

void AItem::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemProperties(State);
}

void AItem::StartItemCurve(APlayerCharacter* Char)
{
	// Store a handle to the Character
	Character = Char;
	// Store initial location of the Item
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	// Setting Item state
	SetItemState(EItemState::EIS_EquipInterping);
	// Start Timer
	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItem::FinishInterping, ZCurveTime);

	// Calculating yaw offset between camera and interp item to stop Item interp rotation when camera rotates
	// Get initial Yaw of the Camera
	const FRotator CameraRotation{ Character->GetFollowCamera()->GetComponentRotation() };
	// Get initial Yaw of the Item
	const FRotator ItemRotation{0.0, CameraRotation.Yaw + InterpInitialYawOffset, 0.0f };
	// Initial Yaw offset between Camera and interp item
	SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);
	
}

