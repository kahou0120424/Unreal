// Fill out your copyright notice in the Description page of Project Settings.


#include "Darchangel_CharacterBase.h"
#include "Runtime/Engine//Classes//Components//BoxComponent.h"

// Sets default values
ADarchangel_CharacterBase::ADarchangel_CharacterBase() :
	right_fist_collision_box(CreateDefaultSubobject<UBoxComponent>(TEXT("RightFistCollisionBox")))
{
		// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
		PrimaryActorTick.bCanEverTick = true;
		if (right_fist_collision_box)
		{
			FVector const extent(8.0f);
			right_fist_collision_box->SetBoxExtent(extent, false);
			right_fist_collision_box->SetCollisionProfileName("NoCollision");
		}

}

// Called when the game starts or when spawned
void ADarchangel_CharacterBase::BeginPlay()
{
	Super::BeginPlay();
	if (right_fist_collision_box)
	{
		FAttachmentTransformRules const rules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			false);
		right_fist_collision_box->AttachToComponent(GetMesh(), rules, "hand_r_socket");
		right_fist_collision_box->SetRelativeLocation(FVector(-7.0f, 0.0f, 0.0f));
	}
	
}

// Called every frame
void ADarchangel_CharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADarchangel_CharacterBase::attack_start()
{
	right_fist_collision_box->SetCollisionProfileName("Fist");
	right_fist_collision_box->SetNotifyRigidBodyCollision(true);
}

void ADarchangel_CharacterBase::attack_end()
{
	right_fist_collision_box->SetCollisionProfileName("NoCollision");
	right_fist_collision_box->SetNotifyRigidBodyCollision(false);
}

