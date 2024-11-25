// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MasterWeapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AProjectileWeapon : public AMasterWeapon
{
	GENERATED_BODY()

public: 
	virtual void Fire(const FVector& HitTarget) override; //esta funcion sobreescribe la funcion original en MasterWeapon.h
private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ServerSideRewindProjectileClass;
};
