// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
	bHealing = true;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCroachSpeed, float BuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCroachSpeed;
	}

	MulticastSpeedBuff(BuffBaseSpeed, BuffCroachSpeed);
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	
	Character->SetHealth(FMath::Clamp(Character->GetHealth()+HealThisFrame, 0, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();
	
	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		HealingRate = 0.f;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UBuffComponent::ResetSpeeds()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

}

/*
* Overlap event solo se ejecuta en el servidor por lo que tenemos que hacer un multicast para propagar el cambio de velocidad en todas las maquinas y que no se vea raro
* (ya que el servidor propaga el movimiento en los clientes pero cuando en los clientes se mueve a diferente velocidad no hay sincronizacion)
*/
void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	HealRampUp(DeltaTime);

}

