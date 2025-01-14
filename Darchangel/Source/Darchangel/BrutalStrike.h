// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrutalStrike.generated.h"

UCLASS()
class DARCHANGEL_API ABrutalStrike : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABrutalStrike();

	UPROPERTY(EditAnywhere, Category = "Damage")
		float Damage;

	UPROPERTY(EditAnywhere, Category = "Life Time")
		float lifeTime;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
		void OnOverlapBegin(class AActor* OverlappedActor, AActor* OtherActor);

	float DeactiveCounter;
	bool deactivate;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void setLifeTime(float LifeTime);



};
