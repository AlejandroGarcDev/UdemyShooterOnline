// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
	if (TrailSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//Instigator = Character que disparo el arma

	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{

		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{

			TArray<AActor*> IgnoredActors;
			IgnoredActors.Add(this);

			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,		//World context object
				Damage,		//BaseDamage
				10.f,		//MinimumDamage
				GetActorLocation(),				//Origin
				200.f,		//DamageInnerRadius
				500.f,		//DamageOuterRadius
				1.f,		//DamageFalloff
				UDamageType::StaticClass(),		//DamageTypeClass
				IgnoredActors,					//IgnoreActors
				this,							//DamageCauser
				FiringController				//InstigatorController
			);
		}
	}
	//Super::OnHit emite un sonido y particulas, ademas de destruir la bala
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}