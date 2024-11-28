// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "UdemyShooterOnline/ShooterComponents/LagCompensationComponent.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"

/*
* Funcion que ejecuta la logica de disparo en un arma HitScan (el propio arma es quien ejecuta el daño, en lugar de lanzar una bala)
*/
void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"); //Boquilla del arma
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation(); //Obtenemos xyz de la boquilla

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit); //Simulamos disparo desde la boquilla hasta la posicion que apuntamos

		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor()); //Comprobamos si hay un character al que hemos disparado
		if (ShooterCharacter && InstigatorController)
		{
			bool bCauseAuthorityDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled(); //Si somos el servidor (LocallyControlled) o si tenemos SSR como false
			if (HasAuthority() && bCauseAuthorityDamage) //Si ha disparado el servidor, aplicamos daño directamente
			{
				const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage; //Si es headshot aplicamos daño de headshot

				UGameplayStatics::ApplyDamage(
					ShooterCharacter,
					DamageToCause,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			if(!HasAuthority() && bUseServerSideRewind) //Si no es el servidor quien disparó y el arma tiene como activo utilizar SSR, se llama al servidor para que compruebe el hit
			{
				ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerPawn) : ShooterOwnerCharacter;
				ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(InstigatorController) : ShooterOwnerController;
				if (ShooterOwnerController && ShooterOwnerCharacter && ShooterOwnerCharacter->GetLagCompensation() && ShooterOwnerCharacter->IsLocallyControlled())
				{
					ShooterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						ShooterCharacter,
						Start,
						HitTarget, //Tambien se puede pasar como parametro FireHit.ImpactPoint
						ShooterOwnerController->GetServerTime() - ShooterOwnerController->SingleTripTime //Aprox del tiempo del server - delay entre cliente-server
					);
				}
			}
		}
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();

	if (World)
	{	
		//Si el arma tiene dispersion, el vector end se genera a traves de la funcion "TraceEndWithScatter"
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);

		FVector BeamEnd = End;//Variable que define donde spawnear las particulas, si ha golpeado a algo se cambia el valor de la variable a FireHit.ImpactPoint
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}