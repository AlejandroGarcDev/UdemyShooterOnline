// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void AProjectileRocket::Destroyed()
{


}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
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

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

/*
* Funcion que aplica daño radial y emite particulas y sonido
* Esta funcion se llama tanto en servidor como en cliente (BeginPlay-Clientes // SuperBeginPlay-Servidor) por lo que el daño radial se tiene 
* que comprobar que sea en servidor unicamente (la aparicion de particulas y sonido si debe de ser en todas las maquinas
*/
void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//Instigator = Character que disparo el arma

	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
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

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
	);

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
}