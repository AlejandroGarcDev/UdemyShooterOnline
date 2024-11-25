// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "UdemyShooterOnline/ShooterComponents/LagCompensationComponent.h"



AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
/*
* Esta funcion refresca los valores que pongas dentro de la funcion cuando cambias un parametro
* En nuestro caso, la velocidad incial del proyectil, ya que depende de la clase Projectile y lo estamos asignado a un componente.
* Al momento de crear el BP se asigna el valor que queremos, pero si queremos cambiar el valor por defecto no se actualiza en el componente projectil
* Entonces, usamos esta funcion para que refresque el valor del componente si se ha cambiado el valor de la variable de la clase (Initial Speed)
*/
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None; //Guarda el nombre de la variable que ha cambiado
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))			//Si dicha variable es Initial Speed, entonces ejecutamo la logica de refresco
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AShooterPlayerController* OwnerController = Cast<AShooterPlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(
					OtherActor,
					Damage,		//Variable definida en Projectile.h
					OwnerController,
					this,
					UDamageType::StaticClass());

				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}

			AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}


	//En super se destruye la propia clase cuando termina el trayecto de la bala, por tanto se llama al final
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	
	/*
			Este codigo permite obtener la trayectoria de la bala (Se utiliza en LagCompensation->ProjectileConfirmHit
			Ideal para saber la trayectoria que ha tenido//tendrá un proyectil

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	PathParams.MaxSimTime = 4.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 30.f;
	PathParams.StartLocation = GetActorLocation();
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this);

	FPredictProjectilePathResult PathResult;

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	*/
}
