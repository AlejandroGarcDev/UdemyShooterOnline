// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"


UCLASS()
class UDEMYSHOOTERONLINE_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AProjectile();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	/*
	*	Used with server-side rewind
	*/

	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity; //2 decimals place of precision

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000;

	//Only set this for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	//Doesnt matter for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

protected:

	virtual void BeginPlay() override;
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void SpawnTrailSystem();
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	//Clase que genera instancias de particulas
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	//Instancia de la clase TrailSystem
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

private:

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;
	
	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;


	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.5f;

};
