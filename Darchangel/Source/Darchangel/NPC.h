// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "NPC.generated.h"

UCLASS()
class DARCHANGEL_API ANPC : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPC();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void melee_attack();
	UAnimMontage* get_montage() const;

	float get_health() const;
	float get_max_health() const;
	void set_health(float const new_health);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	class UWidgetComponent* widget_component;
	float const max_health = 100.0f;
	float health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* montage;

	/*UFUNCTION()
		void on_attack_overlap_begin(
			UPrimitiveComponent* const overlapped_component,
			AActor* const other_actor,
			UPrimitiveComponent* other_component,
			int const other_body_index,
			bool const from_sweep,
			FHitResult const& sweep_result);

	UFUNCTION()
		void on_attack_overlap_end(
			UPrimitiveComponent* const overlapped_component,
			AActor* const other_actor,
			UPrimitiveComponent* other_component,
			int const other_body_index);*/
	

};
