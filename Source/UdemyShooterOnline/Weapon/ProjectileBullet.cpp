// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(
				OtherActor,
				Damage,		//Variable definida en Projectile.h
				OwnerController,
				this,
				UDamageType::StaticClass());
		}
	}


	//En super se destruye la propia clase cuando termina el trayecto de la bala, por tanto se llama al final
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
