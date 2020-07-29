// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "MainCharacter.generated.h"

UCLASS()
class DARCHANGEL_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMainCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		UCameraComponent* FollowCamera;
	

	UPROPERTY(EditAnywhere, Category = "Chains Of Hell")
		float speed;
	UPROPERTY(EditAnywhere, Category = "Chains Of Hell")
		float DistanceBetweenActors = 100;
	UPROPERTY(EditAnywhere, Category = "Chains Of Hell")
		float pullDelay = 3;

	//** Attakc Animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* AttackMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* AttackMontage2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* AttackMontage3;

	//** Character Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CharacterSetting, meta = (AllowPrivateAccess = "true"))
		float AttackDelay;

	void MoveForward(float Axis);
	void MoveRight(float Axis);

	FVector targetLocation;
	FVector playerPos;
	FHitResult OutHit;
	FHitResult OutHit2;
	FVector velocity;
	bool isPull;
	bool isPulling;

	float percent;
	float seconds = 10;
	float timer;

	// dash
	UFUNCTION()
	void Dash();

	UPROPERTY(EditAnywhere)
	float dashDistance;

	UPROPERTY(EditAnywhere)
	float dashCooldown;

	UPROPERTY()
	bool canDash;

	UPROPERTY(EditAnywhere)
	float dashStop;

	UPROPERTY()
	FTimerHandle dashHandle;

	UFUNCTION()
	void StopDash();

	UFUNCTION()
	void ResetDash();

	void AttackMove();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void Raycast();
	void AttackStart();
	bool isAttacking;
	float atkCD;
	float pullCD;
	float atkCount = 0;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	class UAIPerceptionStimuliSourceComponent* stimulus;

	void setup_stimulus();
};
