// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

/*
* Esta funcion refresca los valores que pongas dentro de la funcion cuando cambias un parametro
* En nuestro caso, la velocidad incial del proyectil, ya que depende de la clase Projectile y lo estamos asignado a un componente.
* Al momento de crear el BP se asigna el valor que queremos, pero si queremos cambiar el valor por defecto no se actualiza en el componente projectil
* Entonces, usamos esta funcion para que refresque el valor del componente si se ha cambiado el valor de la variable de la clase (Initial Speed)
*/
#if WITH_EDITOR
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None; //Guarda el nombre de la variable que ha cambiado
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed))			//Si dicha variable es Initial Speed, entonces ejecutamo la logica de refresco
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

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

	SpawnTrailSystem();

}

/*
* Funcion que aplica daño radial y emite particulas y sonido
* Esta funcion se llama tanto en servidor como en cliente (BeginPlay-Clientes // SuperBeginPlay-Servidor) por lo que el daño radial se tiene 
* que comprobar que sea en servidor unicamente (la aparicion de particulas y sonido si debe de ser en todas las maquinas
*/
void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//Instigator = Character que disparo el arma

	ExplodeDamage();

	StartDestroyTimer();

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	//Hacemos invisible el cohete
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
}