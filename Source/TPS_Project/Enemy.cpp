// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blueprint/UserWidget.h"


// Sets default values
AEnemy::AEnemy() : 
	Health(100.0f),
	MaxHealth(100.0f),
	HealthBarDisplayTime(4.0f),
	bCanHitReact(true),
	HitReactTimeMin(0.5f),
	HitReactTimeMax(3.0f),
	bStunned(false),
	StunProbability(0.5f),
	// Montage section names
	AttackLFast(TEXT("AttackLFast")), 
	AttackRFast(TEXT("AttackRFast")),
	AttackL(TEXT("AttackL")),
	AttackR(TEXT("AttackR")),
	// Enemy Damage
	BaseDamage(20.0f),
	bIsDead(false),
	DeathTime(4.0f),
	// Weapon sockets
	LeftWeaponSocket(TEXT("HitSocketL")),
	RightWeaponSocket(TEXT("HitSocketR")),
	bCanAttack(true),
	AttackWaitTime(1.0f),
	HitNumberDestroyTime(1.5f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the Agro Sphere
	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());

	// Add Combat Range Sphere
	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRange"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());

	// Add box coliders for weapon attacks
	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Weapon Box"));
	LeftWeaponCollision->SetupAttachment(GetMesh(), FName("LeftWeaponBone"));
	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Weapon Box"));
	RightWeaponCollision->SetupAttachment(GetMesh(), FName("RightWeaponBone"));
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Bind AgroSphere Overlaps
	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOverlap);

	// Bind Attack Sphere Overlaps
	CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatRangeOverlap);
	CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatRangeEndOverlap);

	// Bind functions to overlap events for weapon box colliders
	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::OnLeftWeaponOverlap);
	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::OnRightWeaponOverlap);

	// Set collision presets for weapon box colliders
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // No collition until attack
	LeftWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Collition type
	LeftWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // Ignore all colition channels
	LeftWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); // only overlap with Pawn collition
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	RightWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);


	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// Set the enemy mesh and capsule collider collisionChanels (layer) to ignore when colliding with the camera
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Get the AI Enemy Controller
	EnemyController = Cast<AEnemyController>(GetController());
	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), true);
	}

	// Transform Local Patro lPoints to World sapece
	FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(),PatrolPoint);
	FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);

	//DrawDebugSphere(GetWorld(), WorldPatrolPoint, 25.0f, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), WorldPatrolPoint2, 25.0f, 12, FColor::Red, true);

	if (EnemyController)
	{
		// Get Balckboard components
		EnemyController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), WorldPatrolPoint);
		EnemyController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint2"), WorldPatrolPoint2);

		// Execute behavior tree
		EnemyController->RunBehaviorTree(BehaviorTree);
	}
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnemy::HideHealthBar, HealthBarDisplayTime);
}

void AEnemy::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	HideHealthBar();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
	
	// if dead set GetBlackboardComponent 'Dead' to true
	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
		EnemyController->StopMovement();
	}
}

void AEnemy::PlayHitMontage(FName Section, float PlayRate)
{
	if (bCanHitReact)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(HitMontage, PlayRate);
			AnimInstance->Montage_JumpToSection(Section, HitMontage);
		}
		bCanHitReact = false;
		const float HitReactTime{ FMath::FRandRange(HitReactTimeMin, HitReactTimeMax) };
		GetWorldTimerManager().SetTimer(HitReactTimer, this, &AEnemy::ResetHitReactTimer, HitReactTime);
	}

}

void AEnemy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

// Sets BlackBoard component "Target" as the playerCharacter
void AEnemy::AgroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr) return;

	auto Character = Cast<APlayerCharacter>(OtherActor);

	if (Character)
	{
		if (EnemyController)
		{
			if (EnemyController->GetBlackboardComponent())
			{
				// Set value of the "Target" Blackboard Key
				EnemyController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), Character);

			}
		}
	}
}

// Handles stunned bool in blackBoard component
void AEnemy::SetStunned(bool Stunned)
{
	bStunned = Stunned;

	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("Stunned"), Stunned);
	}
}

// Handles on entering field of view blackBoard component "InAttackRange" bool
void AEnemy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr) return;

	auto PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (PlayerCharacter)
	{
		bInAttackRange = true;
		if (EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), true);
		}
	}
}

// Handles on exit field of view blackBoard component "InAttackRange" bool
void AEnemy::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr) return;
	auto PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		bInAttackRange = false;
		if (EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), false);
		}
	}
}

// plays attack montage
void AEnemy::PlayAttackMontage(FName Section, float PlayRate)
{

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); 

	if (AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}

	// attack timer logic
	bCanAttack = false;
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::ResetCanAttack, AttackWaitTime); 
	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), false);
	}
}

// Gets random attack section in montage
FName AEnemy::GetAttackSectionName()
{
	FName SectionName;
	const int32 Section{ FMath::RandRange(1, 4) };

	switch (Section)
	{
		case 1:
			SectionName = AttackLFast;
			break;
		case 2:
			SectionName = AttackRFast;
			break;
		case 3:
			SectionName = AttackL;
			break;
		case 4:
			SectionName = AttackR;
			break;
	}
	return SectionName;
}


void AEnemy::DoDamage(APlayerCharacter* Victim)
{
	if (Victim == nullptr) return;

	if (Victim)
	{
		UGameplayStatics::ApplyDamage(Victim, BaseDamage, EnemyController, this, UDamageType::StaticClass());

		if (Victim->GetMeleeImpactSound())
		{
			UGameplayStatics::PlaySoundAtLocation(this, Victim->GetMeleeImpactSound(), GetActorLocation());
		}
	}
}
 
// Blood particles spawner
void AEnemy::SpawnBlood(APlayerCharacter* Victim, FName SocketName)
{
	const USkeletalMeshSocket* TipSocket{ GetMesh()->GetSocketByName(SocketName) };

	if (TipSocket)
	{
		const FTransform SocketTransform{ TipSocket->GetSocketTransform(GetMesh()) };
		if (Victim->GetBloodParticles())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Victim->GetBloodParticles(), SocketTransform);
		}
	}
}

// Reset enemy attack timer
void AEnemy::ResetCanAttack()
{
	bCanAttack = true;
	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), true);
	}
}

void AEnemy::FinishDeath()
{
	GetMesh()->bPauseAnims = true;

	GetWorldTimerManager().SetTimer(DeathTimer, this, &AEnemy::DestroyEnemy, DeathTime);
}

void AEnemy::DestroyEnemy()
{
	Destroy();
}

void AEnemy::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelagate;

	HitNumberDelagate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorld()->GetTimerManager().SetTimer(HitNumberTimer, HitNumberDelagate, HitNumberDestroyTime, false);
}

void AEnemy::DestroyHitNumber(UUserWidget* HitNumber)
{
	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

// Hud Hit Numbers
void AEnemy::UpdateHitNumbers() 
{
	// update location of hit number in map
	for (auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber{ HitPair.Key };
		const FVector Location{ HitPair.Value };
		FVector2D ScreenPosition;

		// Update screen place location
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);

		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

// Weapons Collision logic
void AEnemy::OnLeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, LeftWeaponSocket);
	}
}

void AEnemy::OnRightWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Character = Cast<APlayerCharacter>(OtherActor);
	if (Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, RightWeaponSocket);
	}
}

void AEnemy::ActivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::ActivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHitNumbers();
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::BulletHit_Implementation(FHitResult HitResult, AActor* Player, AController* PlayerController)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.Location, FRotator(0.0f), true);
	}
	
	
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Agro Enemy when damage caused
	if (EnemyController)
	{
		// Set Blackboard comp to target to attack player
		EnemyController->GetBlackboardComponent()->SetValueAsObject(FName("Target"), DamageCauser);
	}
	if (Health - DamageAmount <= 0.0f)
	{
		Health = 0.0f;
		Die();
	}
	else
	{
		Health -= DamageAmount;
	}

	if (bIsDead) return DamageAmount;

	ShowHealthBar();

	// Determine whether bullet hit stuns
	const float Stunned = FMath::FRandRange(0.0f, 1.0f);
	if (Stunned <= StunProbability)
	{
		// Stun Enemy
		PlayHitMontage(FName("HitReactFront"));
		SetStunned(true);
	}

	return DamageAmount;
}

