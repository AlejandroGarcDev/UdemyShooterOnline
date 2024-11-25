// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
	
public:

	AProjectileRocket();

#if WITH_EDITOR
	//Funcion que sirve para que los Blueprints del editor cambien de manera automatica
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

	virtual void Destroyed() override;

protected:

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

private:

};
