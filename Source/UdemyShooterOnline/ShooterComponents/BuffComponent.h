// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UDEMYSHOOTERONLINE_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UBuffComponent();
	friend class AShooterCharacter;

	void Heal(float HealAmount, float HealingTime);
	
	void Shield(float ShieldAmount, float ShieldTime);

	void BuffSpeed(float BuffBaseSpeed, float BuffCroachSpeed, float BuffTime);

	void BuffJump(float BuffJumpVelocity, float BuffTime);

	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float JumpVelocity);
protected:

	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);

	void ShieldRamUp(float DeltaTime);

private:

	UPROPERTY()
	class AShooterCharacter* Character;

	/*
	* Heal Buff
	*/
	bool bHealing = false;
	float HealingRate = 0;
	float AmountToHeal = 0;

	/*
	* Shield Buff
	*/
	bool bShield = false;
	float ShieldRate = 0;
	float AmountToShield = 0;

	/*
	* Speed Buff
	*/
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();

	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/*
	* Jump Buff
	*/
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
public:	

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
