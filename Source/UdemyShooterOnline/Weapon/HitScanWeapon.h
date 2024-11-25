// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MasterWeapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AHitScanWeapon : public AMasterWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitTarget) override;

protected:

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;


private:

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	//En caso de no tener animacion en el arma que tenga sonido, utilizamos esta variable para tener un sonido al disparar
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

};
