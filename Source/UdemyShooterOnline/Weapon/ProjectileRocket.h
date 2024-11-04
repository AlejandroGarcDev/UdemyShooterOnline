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

	virtual void Destroyed() override;

protected:

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	void DestroyTimerFinished();

	//Clase que genera instancias de particulas
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	//Instancia de la clase TrailSystem
	class UNiagaraComponent* TrailSystemComponent;
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;


	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.5f;

public:

	FORCEINLINE 
};
