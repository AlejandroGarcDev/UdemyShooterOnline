// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/ShooterComponents/LagCompensationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AMasterWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		//En este TMap se hara un registro de las balas que golpearon a jugador, por ejemplo: Character2 - 5: el character 2 recibio 5 disparos
		TMap<AShooterCharacter*, uint32> HitMap;
		TMap<AShooterCharacter*, uint32> HeadShotHitMap;
		//Recorremos el vector que pasamos como parametro para hacer los checks de a donde golpea
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			//Compruebo si el actor golpeado es un character
			AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor());
			if (ShooterCharacter)
			{
				//Segun sea headshot o no, se agrega a un TMap diferente
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");
				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(ShooterCharacter)) HeadShotHitMap[ShooterCharacter]++;
					else HeadShotHitMap.Emplace(ShooterCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(ShooterCharacter)) HitMap[ShooterCharacter]++;
					else HitMap.Emplace(ShooterCharacter, 1);
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
						FireHit.ImpactPoint,
						0.5f,
						FMath::FRandRange(-0.5f, 0.5f)
					);
				}

			}
		}

		TArray<AShooterCharacter*> HitCharacters; //Este array sirve para, en caso de que no sea el servidor quien haya disparado, llamar al servidor con todos
												  //Los characters golpeados y sea el servidor quien verifique el disparo

		TMap<AShooterCharacter*, float> DamageMap; //Este TMap sirve para acumular los bodyshots y los headshots

		//Recorre el TMap de daño del cuerpo y suma cada golpe al TMap DamageMap en funcion de los conteos de golpes que tenga
		for (auto HitPair : HitMap)
		{
			//Key devuelve AShooterCharacter*
			if (HitPair.Key && InstigatorController)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);
				HitCharacters.AddUnique(HitPair.Key);

			}
		}
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			//Key devuelve AShooterCharacter*
			if (HeadShotHitPair.Key)
			{
				//Si ya existe ese character en DamageMap se agrega el daño del headshot, si no, se crea el character con el daño de los posibles headshots
				if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key); //AddUnique tiene en cuenta los elementos existentes, por lo que si el character no fue agregado en el for
															  //de los bodyshots, se agrega en esta linea en caso de que haya sigo golpeado en la cabeza

			}
		}

		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthorityDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled(); //Si somos el servidor (LocallyControlled) o si tenemos SSR como false
				if (HasAuthority() && bCauseAuthorityDamage)//Si es el servidor quien disparo, se aplica daño automaticamente, pues no tiene lag y no hay que verificar nada
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key,	    //Character that was hit
						DamagePair.Value,	//Damage calculated in the two for loops above
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}




		if (!HasAuthority() && bUseServerSideRewind)//Si no fue el serv. quien disparo, se llama a ShotgunServerSocreRequest
		{
			ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerPawn) : ShooterOwnerCharacter;
			ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(InstigatorController) : ShooterOwnerController;
			if (ShooterOwnerController && ShooterOwnerCharacter && ShooterOwnerCharacter->GetLagCompensation() && ShooterOwnerCharacter->IsLocallyControlled())
			{
				ShooterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start, 
					HitTargets,
					ShooterOwnerController->GetServerTime() - ShooterOwnerController->SingleTripTime
				);
			}
		}
	}
}

/*
* Funcion que recoge diferentes puntos aleatorios y los guarda en OutHitTargets
* Puntos aleatorios en funcion de:
* 1) NumberOfPellets
* 2) DistanceToSphere
* 3) SphereRadius
*/
void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	//Se normaliza la distancia a la que apunta para despues generar una esfera de referencia a la hora de generar trayectorias aleatorias
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		//Genero la localizacion random dentro de la esfera
		const FVector RandomVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		//Añado offset o posicion de origen de la esfera al punto aleatorio generado
		const FVector EndLoc = SphereCenter + RandomVec;
		//Genero el vector de desplazamiento de la bala
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGHT / ToEndLoc.Size();

		OutHitTargets.Add(ToEndLoc);
	}
}
