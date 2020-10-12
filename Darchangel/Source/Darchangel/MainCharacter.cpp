// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "cmath"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "Engine.h"
#include "CableComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Bullet.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "ai_tags.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "HealthBar.h"
#include "Runtime/Engine/Classes/Components/BoxComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "NPC.h"
#include "BrutalStrike.h"
#include "Chain.h"
#include "GraspofDeath.h"
#include "WallOfLight.h"
#include "BlessedIdol.h"
#include "ChargeParticle.h"
#include "Kismet/GameplayStatics.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::Green,text)

///////////////////////////////////////////////////////////////////////
//Distance Formula
double Distance(FVector a, FVector b)
{
	double distance = abs(sqrt(pow(b.X - a.X, 2) + pow(b.Y - a.Y, 2)));
	return distance;
}
///////////////////////////////////////////////////////////////////////

// Sets default values
AMainCharacter::AMainCharacter() :
	health(max_health),
	//widget_component(CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthValue"))),
	sword_collision_box(CreateDefaultSubobject<UBoxComponent>(TEXT("SwordCollisionBox"))),
	Cable(CreateDefaultSubobject<UCableComponent>(TEXT("Cable")))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f); //Capsule Size

	bUseControllerRotationPitch = false; // Not allowing the character to rotate
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; // allow the charactor ratote to the direction it is moving  
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 1200.0f, 0.0f); //rotation rate
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->BrakingFrictionFactor = 0.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // name of the component
	CameraBoom->SetupAttachment(RootComponent); // child of the RootComponent


	CameraBoom->TargetArmLength = 300.0f; // how far away the camera from the player
	CameraBoom->bUsePawnControlRotation = true; //rotate the arm base of the controller 

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // attact the camera on the end of the boom, let the boom adjust the mass controller rotation of the camera
	FollowCamera->bUsePawnControlRotation = false; // Camera did not rotate relative to the r

	setup_stimulus();
	/*if (widget_component)
	{
		widget_component->SetupAttachment(RootComponent);
		widget_component->SetWidgetSpace(EWidgetSpace::Screen);
		widget_component->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
		static ConstructorHelpers::FClassFinder<UUserWidget> widget_class(TEXT("/Game/UI/HealthBar_BP"));
		if (widget_class.Succeeded())
		{
			widget_component->SetWidgetClass(widget_class.Class);
		}
	}*/

	if (sword_collision_box)
	{
		FVector const extent(50.0f);
		sword_collision_box->SetBoxExtent(extent, false);
		sword_collision_box->SetCollisionProfileName("NoCollision");
	}




}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	FAttachmentTransformRules const rules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		false);

	HideWeapon = false;
	if (sword_collision_box)
	{

		sword_collision_box->AttachToComponent(GetMesh(), rules, "Sword_Middle");
		sword_collision_box->SetRelativeLocation(FVector(-7.0f, 0.0f, 0.0f));

	}

	if (Cable)
	{
		Cable->AttachToComponent(GetMesh(), rules, "index_01_l");
	}

	if (sword_collision_box)
	{
		sword_collision_box->OnComponentBeginOverlap.AddDynamic(this, &AMainCharacter::on_attack_overlap_begin);
		sword_collision_box->OnComponentEndOverlap.AddDynamic(this, &AMainCharacter::on_attack_overlap_end);
	}

	isDemon = true;
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*(auto const uw = Cast<UHealthBar>(widget_component->GetUserWidgetObject());
	if (uw)
	{
		uw->set_bar_value_percent(health / max_health);
	}*/

	PullingCoolDownFunction(DeltaTime);

	BulletRateFunction();
	
	StopCharacter();

	StrongRangeChecker(DeltaTime);
	
	AttackStateCounterFunction(DeltaTime);
}

void AMainCharacter::RotateToMouseCurse()
{
	APlayerController* playerController = (APlayerController*)GetWorld()->GetFirstPlayerController();
	bool isHit2 = playerController->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, OutHit2);
	FRotator rotatePoint = UKismetMathLibrary::FindLookAtRotation(this->GetActorLocation(), OutHit2.Location);
	this->SetActorRotation(FRotator(0, rotatePoint.Yaw, 0));
}

void AMainCharacter::Raycast() //Chain Of Hell
{
	/*if (!isPulling && !canJumpWall)
	{

		RotateToMouseCurse();

		const FRotator SpawnRotation = GetActorRotation();
		const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			AChain* chain = World->SpawnActor<AChain>(ChainProjectileClass, SpawnLocation, SpawnRotation);
			Cable->SetAttachEndTo(chain, "Sphere");
			Cable->SetVisibility(true);
			GetWorldTimerManager().SetTimer(chainHandle, this, &AMainCharacter::HideCable, 0.7f, false);
			isPulling = true;
		}
	}*/
	RotateToMouseCurse();
	if(canJumpWall)
	{
		JumpUp();
	}

}

void AMainCharacter::MeleeAttack() // Melee Attack
{
	if (!ForceStop)
	{
		RotateToMouseCurse();
		if (isDemon)
		{
			if (atkCount == 0)
			{
				PlayAnimMontage(DemonAttackMontage1, 1.f);
				atkCount++;
			}
			else if (atkCount == 1)
			{
				PlayAnimMontage(DemonAttackMontage2, 1.f);
				atkCount++;

			}
			else if (atkCount == 2)
			{
				PlayAnimMontage(DemonAttackMontage3, 1.f);
				atkCount = 0;
			}
			
		}
		else
		{
			if (atkCount == 0)
			{
				PlayAnimMontage(AngelAttackMontage1, 1.f);
				atkCount++;
			}
			else if (atkCount == 1)
			{
				PlayAnimMontage(AngelAttackMontage2, 1.f);
				atkCount = 0;

			}
			
		}
		AttackStateCounter = 0;
		IsAttackState = true;
		ForceStop = true;
	}

}
void AMainCharacter::RangeAttack() // Range Attack
{

	if (BulletProjectileClass != NULL)
	{
		RotateToMouseCurse();
		const FRotator SpawnRotation = GetActorRotation();
		const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			ABullet* Bullet = World->SpawnActor<ABullet>(BulletProjectileClass, SpawnLocation, SpawnRotation);

			FVector NewVelocity = GetActorForwardVector() * 2000.0f;
			Bullet->Velocity = FVector(NewVelocity);
		}

	}
	isRangeHolding = true;

}
void AMainCharacter::StrongRangeAttack() // Range Attack
{
	if (rangeHoldTimer > rangeHoldTime)
	{
		if (EnhanceBulletProjectileClass != NULL)
		{
			RotateToMouseCurse();
			const FRotator SpawnRotation = GetActorRotation();
			const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100);

			UWorld* const World = GetWorld();
			if (World != NULL)
			{
				ABullet* Bullet = World->SpawnActor<ABullet>(EnhanceBulletProjectileClass, SpawnLocation, SpawnRotation);

				FVector NewVelocity = GetActorForwardVector() * 2000.0f;
				Bullet->Velocity = FVector(NewVelocity);
			}

		}
	}
	isRangeHolding = false;


}



// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	PlayerInputComponent->BindAction("ChainsOfHell", IE_Pressed, this, &AMainCharacter::Raycast);
	PlayerInputComponent->BindAction("Normal Attack", IE_Pressed, this, &AMainCharacter::MeleeAttack);
	PlayerInputComponent->BindAction("Distract", IE_Pressed, this, &AMainCharacter::on_distract);
	PlayerInputComponent->BindAction("Strong Attack", IE_Pressed, this, &AMainCharacter::PlayChargingAnimation);
	PlayerInputComponent->BindAction("Strong Attack", IE_Released, this, &AMainCharacter::PlayStrongAttackAnimation);
	PlayerInputComponent->BindAction("BrutalStrike/Wall Of Light", IE_Pressed, this, &AMainCharacter::BrutalStrikeAnimation);
	PlayerInputComponent->BindAction("Grasp of Death / Blessed Idol", IE_Pressed, this, &AMainCharacter::GraspOfDeathAnimation);
	PlayerInputComponent->BindAction("Swap Form", IE_Pressed, this, &AMainCharacter::SwapForm);

}

void AMainCharacter::setup_stimulus()
{
	stimulus = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("stimulus"));
	stimulus->RegisterForSense(TSubclassOf < UAISense_Sight>());
	stimulus->RegisterWithPerceptionSystem();
}

void AMainCharacter::on_distract()
{
	if (distraction_sound)
	{
		FVector const loc = GetActorLocation();
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), distraction_sound, loc);
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), loc, 1.0f, this, 0.0f, tags::noise_tag);
	}
}

void AMainCharacter::MoveForward(float Axis)
{
	if (!ForceStop )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Axis);
	}


}

void AMainCharacter::MoveRight(float Axis)
{
	if (!ForceStop)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Axis);

	}

}




void AMainCharacter::AttackMove() // Move forward while attacking
{
	GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
	LaunchCharacter(FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0).GetSafeNormal() * attackForwardDistance, true, true);
}

void AMainCharacter::Fire()
{
	isShooting = true;
}

void AMainCharacter::FinishFire()
{
	isShooting = false;
}

float AMainCharacter::get_health() const
{
	return health;
}

float AMainCharacter::get_max_health() const
{
	return max_health;
}

void AMainCharacter::set_health(float const new_health)
{
	health = new_health;
	if (health <= 0)
	{
		auto const controller = UGameplayStatics::GetPlayerController(this, 0);
		controller->SetCinematicMode(true, false, false, true, true);
		GetMesh()->SetSimulatePhysics(true);
		GetMovementComponent()->MovementState.bCanJump = false;
	}
}

void AMainCharacter::attack_start()
{
	sword_collision_box->SetCollisionProfileName("Sword");
	sword_collision_box->SetNotifyRigidBodyCollision(true);
}

void AMainCharacter::attack_end()
{
	sword_collision_box->SetCollisionProfileName("NoCollision");
	sword_collision_box->SetNotifyRigidBodyCollision(false);
}

void AMainCharacter::on_attack_overlap_begin(
	UPrimitiveComponent* const overlapped_component,
	AActor* const other_actor,
	UPrimitiveComponent* other_component,
	int const other_body_index,
	bool const from_sweep,
	FHitResult const& sweep_result)
{
	if (ANPC* const npc = Cast<ANPC>(other_actor))
	{
		print("Hit");
		float const new_health = npc->get_health() - npc->get_max_health() * 0.2f;
		npc->set_health(new_health);
	}
}

void AMainCharacter::on_attack_overlap_end(
	UPrimitiveComponent* const overlapped_component,
	AActor* const other_actor,
	UPrimitiveComponent* other_component,
	int const other_body_index)
{

}

void AMainCharacter::WallJumpStart(FVector jumpLocation)
{
	canJumpWall = true;
	jumpPos = jumpLocation;
}

void AMainCharacter::WallJumpEnd()
{
	canJumpWall = false;
}




void AMainCharacter::JumpUp()
{
	if (!canJumpWall)
		return;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo(RootComponent, jumpPos, FRotator(0.0f, 0.0f, 0.0f), false, false, 0.5f, false, EMoveComponentAction::Type::Move, LatentInfo);
}


void AMainCharacter::Rope()
{
	FVector velocity2 = jumpPos - GetActorLocation();

	LaunchCharacter(velocity2 * 7.5, true, true);
}

void AMainCharacter::HideCable()
{
	Cable->SetVisibility(false);
	stopMoving = false;
}


void AMainCharacter::BrutalStrikeAnimation()
{
	if (!isDemon)
	{
		WallOfLightFunction();
		return;
	}
		

	if (!BrutalStrikeInCD)
	{
		
		PlayAnimMontage(BrutalStrikeMontage, 1.f, FName("Brutal_Strike_Animation"));
		ForceStop = true;
		GetWorldTimerManager().SetTimer(brutalStrikeCDHandle, this, &AMainCharacter::FinishBrutalStrikeCD, BrutalStrikeCD, false);
		BrutalStrikeInCD = true;
	}
	
}

void AMainCharacter::BrutalStikeFunction()
{
	if (BurtalStrikeTriggerBox != NULL)
	{
		
		BrutalStrikeSpawnRotation = GetActorRotation();
		PlayerLocation = GetActorLocation();
		PlayerForwardPosition = GetActorForwardVector();
		const FVector SpawnLocation = PlayerLocation;
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			ABrutalStrike* brutalStikeProjectile = World->SpawnActor<ABrutalStrike>(BurtalStrikeTriggerBox, SpawnLocation, BrutalStrikeSpawnRotation);		
		}
	}		
}

void AMainCharacter::GraspOfDeathFunction()
{
	
	if (GraspOfDeathProjectile != NULL)
	{
		const FRotator SpawnRotation = GetActorRotation();
		const FVector SpawnLocation = GetActorLocation();

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			AGraspofDeath* projectTile = World->SpawnActor<AGraspofDeath>(GraspOfDeathProjectile, SpawnLocation, SpawnRotation);
		}
		
	}	
	
}



void AMainCharacter::WallOfLightFunction()
{
	RotateToMouseCurse();
	if (WallOfLightProjectile != NULL)
	{
		const FRotator SpawnRotation = GetActorRotation();
		const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			AWallOfLight* projectTile = World->SpawnActor<AWallOfLight>(WallOfLightProjectile, SpawnLocation, SpawnRotation);
			FVector NewVelocity = GetActorForwardVector() * WallOfLightFireRate;
			projectTile->Velocity = FVector(NewVelocity);
			projectTile->CharacterForwardPosition = GetActorForwardVector();
		}
	}
}

void AMainCharacter::SwapForm()
{
	if (ForceStop)
		return;
	atkCount = 0;
	if (isDemon)
		isDemon = false;
	else
		isDemon = true;
}



void AMainCharacter::FinishBrutalStrikeCD()
{
	BrutalStrikeInCD = false;
}

void AMainCharacter::FinishGrashofDeathCD()
{
	GrashofDeathInCD = false;
}

void AMainCharacter::GraspOfDeathAnimation()
{
	if (!isDemon)
	{
		PlayBlessedIdolAnimation();
		return;
	}
	
	if (!GrashofDeathInCD)
	{
		PlayAnimMontage(GraspOfDeathMontage, 1.f, FName("Grasp_Of_Death_Animation"));
		ForceStop = true;
		GetWorldTimerManager().SetTimer(grashofDeathCDHandle, this, &AMainCharacter::FinishGrashofDeathCD, GrashofDeathCD, false);
		GrashofDeathInCD = true;
	}
}

void AMainCharacter::FinishActtack()
{
	ForceStop = false;
}

void AMainCharacter::HideWeaponFunction()
{
	if (HideWeapon)
		HideWeapon = false;
	else
		HideWeapon = true;
}

void AMainCharacter::BlessedIdolFunction()
{
	if (!BlessedIdolInCD)
	{
		if (BlessedIdolProjectile != NULL)
		{
			RotateToMouseCurse();
			const FRotator SpawnRotation = GetActorRotation();
			const FVector SpawnLocation = this->GetActorLocation() + (GetActorForwardVector() * 100);

			UWorld* const World = GetWorld();
			if (World != NULL)
			{
				ABlessedIdol* projectTile = World->SpawnActor<ABlessedIdol>(BlessedIdolProjectile, SpawnLocation, SpawnRotation);
				FVector NewVelocity = GetActorForwardVector() * BlessedIdolFIreRate;
				projectTile->Velocity = FVector(NewVelocity);
			}
			BlessedIdolInCD = true;
			GetWorldTimerManager().SetTimer(BlessedIdolCDHandle, this, &AMainCharacter::FinishBlessedIdolCD, BleesedIdolCooldown, false);
		}
	}
	
}




void AMainCharacter::AttackStateCounterFunction(float Deltatime)
{
	if (IsAttackState)
	{		
		AttackStateCounter = AttackStateCounter + Deltatime;
		if (AttackStateCounter >= 5.0)
			EndAttackState();
	}

}

void AMainCharacter::PullingCoolDownFunction(float DeltaTime)
{
	if (isPulling == true)
	{
		pullCD += DeltaTime;
		if (pullCD >= pullDelay)
		{
			isPulling = false;
			pullCD = 0;
		}
	}
}

void AMainCharacter::BulletRateFunction()
{
	if (isShooting)
	{
		BulletRateCounter++;
		if (BulletRateCounter >= bulletRate)
		{
			RangeAttack();
			BulletRateCounter = 0;
		}
	}
}

void AMainCharacter::StopCharacter()
{
	if (stopMoving)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}



void AMainCharacter::StrongRangeChecker(float DeltaTime)
{
	if (isRangeHolding)
	{
		rangeHoldTimer += DeltaTime;
	}
	else
	{
		rangeHoldTimer = 0;
	}
}

void AMainCharacter::FinishBlessedIdolCD()
{
	BlessedIdolInCD = false;
}



void AMainCharacter::EndAttackState()
{
	IsAttackState = false;
}

void AMainCharacter::StrongAttackState()
{
	if (isDemon)
	{
		if (!StrongAttackStateTwo)
		{
			StrongAttackStateTwo = true;
			return;
		}
		else
		{
			StrongAttackStateTwo = false;
			StrongAttackStateThree = true;
		}
	}
	else
	{
		if (!StrongAttackStateTwo)
		{
			StrongAttackStateTwo = true;
			return;
		}
	}
	

}

void AMainCharacter::PlayStrongAttackAnimation() // Melee Attack
{
	if (!IsMeleeCharging)
		return;

	AttackStateCounter = 0;
	IsAttackState = true;

	if (SpawnedParticle == true)
	{
		ChargePaticleReference->Destroy();
		SpawnedParticle = false;
	}

	if (isDemon)
	{
		if (StrongAttackStateTwo)
		{
			PlayAnimMontage(StrongAttackStateTwoMontage, 1.0f);

			FVector StartPosition = this->GetActorLocation();
			FVector EndPosition = StartPosition + FVector(1.0, 0.0, 0.0);
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(GetOwner());

			TArray<FHitResult> OutHits;

			bool hit = UKismetSystemLibrary::SphereTraceMulti(
				GetWorld(),
				StartPosition,
				EndPosition,
				200.0f,
				TraceTypeQuery1,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::None,
				OutHits,
				true
			);

			if (hit)
			{
				for (int q = 0; q < OutHits.Num(); q++)
				{
					if (OutHits[q].GetActor() != NULL && OutHits[q].Actor->ActorHasTag("Enemy"))
					{
						if (ANPC* const npc = Cast<ANPC>((OutHits[q].GetActor())))
						{
							print("Hit");
							float const new_health = npc->get_health() - npc->get_max_health() * 0.2f;
							npc->set_health(new_health);
						}
					}
				}
			}
		}
		else if (StrongAttackStateThree)
		{
			PlayAnimMontage(StrongAttackStateThreeMontage, 1.0f);

			FVector StartPosition = this->GetActorLocation();
			FVector EndPosition = StartPosition + FVector(1.0, 0.0, 0.0);
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(GetOwner());

			TArray<FHitResult> OutHits;

			bool hit = UKismetSystemLibrary::SphereTraceMulti(
				GetWorld(),
				StartPosition,
				EndPosition,
				300.0f,
				TraceTypeQuery1,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::None,
				OutHits,
				true
			);

			if (hit)
			{
				for (int q = 0; q < OutHits.Num(); q++)
				{
					if (OutHits[q].GetActor() != NULL && OutHits[q].Actor->ActorHasTag("Enemy"))
					{
						if (ANPC* const npc = Cast<ANPC>((OutHits[q].GetActor())))
						{
							print("Hit");
							float const new_health = npc->get_health() - npc->get_max_health() * 0.2f;
							npc->set_health(new_health);
						}
					}
				}
			}
		}
		else
		{
			PlayAnimMontage(StrongAttackStateOneMontage, 1.0f);
		}
	}
	else
	{
		if (StrongAttackStateTwo)
		{
			PlayAnimMontage(AngelStrongAttack2Montage, 1.0f);
		}
		else
		{
			PlayAnimMontage(AngelStrongAttack1Montage, 1.0f);
		}
	}
	
	
	StrongAttackStateTwo = false;
	StrongAttackStateThree = false;
	IsMeleeCharging = false;
}

void AMainCharacter::PlayChargingAnimation()
{
	if (ForceStop)
		return;
	if (isDemon)
	{
		PlayAnimMontage(DemonChargeMontage, 1.0f);
		GetWorldTimerManager().SetTimer(ChargeParticleDelay, this, &AMainCharacter::SpawnChargeParticle, 0.4, false);
	}
		
	else
		PlayAnimMontage(AngelChargeMontage, 1.0f);
	ForceStop = true;
	IsMeleeCharging = true;
}

void AMainCharacter::PlayBlessedIdolAnimation()
{
	PlayAnimMontage(BlessedIdolMontage, 1.0f);
	ForceStop = true;
}

void AMainCharacter::SpawnChargeParticle()
{
	if (!IsMeleeCharging)
		return;

	const FRotator SpawnRotation = GetActorRotation();
	const FVector SpawnLocation = FVector(this->GetActorLocation().X, this->GetActorLocation().Y, this->GetActorLocation().Z - 80.0f);

	UWorld* const World = GetWorld();

	if (World != NULL)
	{
		ChargePaticleReference = World->SpawnActor<AChargeParticle>(ChargeParticle, SpawnLocation, SpawnRotation);
		SpawnedParticle = true;
	}
}
