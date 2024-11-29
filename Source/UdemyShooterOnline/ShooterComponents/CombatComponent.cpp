// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "UdemyShooterOnline/Weapon/MasterWeapon.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "UdemyShooterOnline/GameMode/TeamsGameMode.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"
#include "UdemyShooterOnline/Weapon/Shotgun.h"
#include "UdemyShooterOnline/Weapon/Flag.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAiming = false;
	
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 200.f;
	// ...

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); //Al poner _Condition, podemos poner un condicional en la replicacion,
																			//En este caso COND_OwnerOnly hace que se replique desde el servidor hasta el
																			//unico cliente posee el character de este combatcomponent
																			//Esto esta hecho así porque la municion de cada personaje no hace falta replicarlo
																			//A todos los clientes

	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
	// ...
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);	//Comprueba donde chocaria la bala si se dispara
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);		//Set Crosshair en funcion del arma, de si dispara o salta
		InterpFOV(DeltaTime);				//Set CameraFOV en funcion de si apunta
	}
}


void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	//Si controller es null recogemos el controlador del character, si tiene un valor, se le asigna su mismo valor (no cambia)
	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{	
		//El crosshair se establece segun el arma que tenemos equipada
		HUD = HUD == nullptr ? Cast<AShooterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
				HUDPackage.CrosshairTop = nullptr;
			}

			//Calculate crosshair spread -> Dispersion del crosshair

			// Max speed from 0 to 1 [0,600] -> [0,1]
			// Recogemos los parametros para clampear la velocidad del jugador

			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, CrosshairInAirFactorMAX, DeltaTime, 2.2f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				if (EquippedWeapon)
				{
					float MoveAimFactor; //Relacion para que el apuntar funcione si estas quieto
					MoveAimFactor = 1 - CrosshairVelocityFactor;	// MoveAimFactor = 0 ==> CrosshairVelocity = 1 ;
																	// MoveAimFactor = 1 ==> CrosshairVelocity = 0 ;

					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, CrosshairAimFactorMAX * MoveAimFactor, DeltaTime, EquippedWeapon->CrosshairAimInterpSpeed);
				}
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 20.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, CrosshairShootInterpSpeed);

			HUDPackage.CrosshairSpread =
				CrosshairAimFactorMAX -
				CrosshairAimFactor +
				CrosshairVelocityFactor +
				CrosshairInAirFactor +
				CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}

}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}


//Esta variable llama a la funcion RPC para que ejecute la propagacion a todos los clientes de la variable IsAiming
//Por como funciona los RPC, da igual si la funcion la ejecuta el cliente o el servidor, ya que en ambos casos la funcion será ejecutada en el servidor
void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	//Al setear nuestra variable antes de que el RPC se ejecute, veremos en local antes que los otros jugadores como apuntamos,
	//Esto no afecta al rendimiento del juego porque es simplemente el cambio de una pose y haremos que la experiencia sea mas fluida
	bAiming = bIsAiming;

	//Seteamos la velocidad del personaje en funcion de si está apuntando o no.
	if (Character)
	{
			//Seteo la velocidad del personaje en funcion de si esta apuntando
			Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	//Una vez hemos actualizado el bool de apuntar y la velocidad, propagamos esa informacion al cliente.
	ServerSetAiming(bIsAiming);
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
	if(Character->IsLocallyControlled()) bAimButtonPressed = bIsAiming;
}

//Funcion RPC (Remote Procedure Calls: Funciones que se llaman en cliente pero se ejecutan en el servidor) Esta funcion servira
//para que la animacion de apuntar se propague desde el cliente "A" hacia el servidor y este progague al resto de clientes
//que el personaje A ha apuntado o no.
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	this->bAiming = bIsAiming;

	//Seteamos la velocidad del personaje en funcion de si está apuntando o no pero esta vez en el servidor, de esta forma hay sincronizacion de las variables.
	//Si omitimos esta parte del codigo, el personaje no haria caso al codigo local y seguiria yendo a la velocidad que marca el servidor, ya que tiene mas autoridad
	if (Character)
	{
		//Seteo la velocidad del personaje en funcion de si esta apuntando
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		PlayEquipWeaponSound(EquippedWeapon);
		EquippedWeapon->EnableCustomDepth(false); //Esto se podria quitar ya que SetWeaponState ya desactiva el customdepth
		EquippedWeapon->SetHUDAmmo();
	}
}


void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Secondary);
		AttachActorToBack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
	}
	if (!Character->IsLocallyControlled())
	{
		SecondaryWeapon->EnableCustomDepth(false);
	}
}

/*
* Esta funcion lanza un rayo desde el centro de la pantalla simulando la trayectoria de una bala
*/
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	//Esta funcion convierte un punto de la pantalla en un punto del mundo y una direccion
	bool bScreenToWorld =	UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if(Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			//Se le suma a start la distancia entre la camara y el jugador para que la linea empiece delante del player ; margen de seguridad 100.f
			//CrosshairWorldDirection == direccion a la que apunta el crosshair
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGHT;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		
		//Si implementa la interfaz InteractWithCrosshair para ver si es enemigo o aliado
		//(al momento de escribir esto solo ShooterCharacter implementa esa interfaz)

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			ATeamsGameMode* TeamGameMode = Cast<ATeamsGameMode>(Character->GetGameMode());
			if (TeamGameMode) //Compruebo si estan en duelo por equipos
			{
				AShooterCharacter* CharacterAimed = Cast<AShooterCharacter>(TraceHitResult.GetActor());
				if (CharacterAimed && CharacterAimed->GetPlayerState() && Character && Character->GetPlayerState())
				{
					ETeam AimedTeam = CharacterAimed->GetShooterPlayerState()->GetTeam();
					ETeam OwnTeam = Character->GetShooterPlayerState()->GetTeam();
					if (AimedTeam == OwnTeam) //Compruebo si son del mismo equipo
					{
						HUDPackage.CrosshairColor = FLinearColor::Green;
					}
					else
					{
						HUDPackage.CrosshairColor = FLinearColor::Red;
					}
				}
				else
				{
					HUDPackage.CrosshairColor = FLinearColor::White; //si falla el if, no es un enemigo (al momento de escribir este codigo)
				}
			}
			else //Si no estan en ese modo, todo Shooter Character es enemigo
			{
				HUDPackage.CrosshairColor = FLinearColor::Red;
			}
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}

		//Por como funciona el LineTrace, si no ha chocado con nada, no se rellena el impact point, el cual es un vector2d
		//Nosotros necesitamos esa informacion aunque no choque con nada para llevar los proyectiles (cohetes, granadas, balas etc...), 
		//Asi que lo gestionamos manualmente
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = Start + CrosshairWorldDirection * 2500.f; //Distancia de la bala en caso de que no apunte a nada
		}
	}
}



/*
* Funcion que se ejecuta en el character que esté disparando, esta funcion llamara al servidor para que propague la funcion a todas las maquinas conectadas
*/
void UCombatComponent::FireButtonPressed(bool bPressed)
{

	bFireButtonPressed = bPressed;
	if (EquippedWeapon)
	{
		if (bFireButtonPressed)
		{
			Fire();
		}
		else
		{
			if (!bFireOnCooldown)
			{
				bCanFire = true;
			}
		}
	}
}

/*
* Funcion que se ejecuta cuando se recoge municion (Es llamada desde AmmoPickup)
*/
void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo); //Notice no limit ammo
		UpdateCarriedAmmo();
	}

	//If we didnt have ammo, we reload automatically
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = EquippedWeapon->GetCrosshairShootingSpread(); //Factor que desestabiliza la mira cuando se dispara, depende del propio arma
			CrosshairShootInterpSpeed = EquippedWeapon->GetCrosshairShootInterpSpeed(); //Velocidad con la que se reajusta el crosshair al disparar, depende del propio arma
		
			UE_LOG(LogTemp, Warning, TEXT("CombatComponent->Fire()->HitTarget: (%d,%d,%d)"), HitTarget.X, HitTarget.Y, HitTarget.Z);

			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			}

		}
		StartFireTimer();
	}	
}

/*
* Funcion que ejecuta la logica de disparar un arma que usa proyectiles, Logica:
* 1) Comprobar si usa dispersion y generar el punto aleatorio
* 2) Generar efectos visuales localmente
* 3) Mandarla el resultado de la dispersion servidor para que procese el disparo y efectos visuales en el resto de maquinas
*/
void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		if (EquippedWeapon->bUseScatter)
		{
			HitTarget = EquippedWeapon->TraceEndWithScatter(HitTarget);
		}
		LocalFire(HitTarget);	//Llamamos a la maquina local para que cree lo visual mas rapido 
		ServerFire(HitTarget);	//Llamamos al servidor para que gestione daño
	}
}

/*
* Funcion que ejecuta la logica de disparar un arma con scaterring
* 1) Generar la dispersion localmente
* 2) Generar efectos visuales localmente
* 3) Mandarla el resultado de la dispersion servidor para que procese el disparo y efectos visuales en el resto de maquinas
*/
void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		LocalFire(HitTarget);	//Llamamos a la maquina local para que cree lo visual mas rapido 
		ServerFire(HitTarget);	//Llamamos al servidor para que gestione daño
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		LocalShotgunFire(HitTargets);
		ServerShotgunFire(HitTargets);
	}
	
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)	return;

	bFireOnCooldown = true;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,									//Parametro que representa donde se guarda el timer
		this,										//Objeto que llama al timer
		&UCombatComponent::FireTimerFinished,		//Funcion que se ejecuta al terminar el timer
		EquippedWeapon->FireDelay);					//Tiempo del timer
}

void UCombatComponent::FireTimerFinished()
{
	bFireOnCooldown = false;

	if (EquippedWeapon == nullptr) return;

	//Arma automatica->Puede disparar siempre, y si dejó el boton de disparar apretado, dispara
	if (EquippedWeapon->bAutomatic)
	{
		bCanFire = true;
		if (bFireButtonPressed)
		{
			Fire();
		}
	}
	//Arma manual->Solo puede disparar si tiene el boton sin apretar
	else
	{
		if (!bFireButtonPressed)
		{
			bCanFire = true;
		}
	}

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (bLocallyReloading) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{

		Controller->SetHUDCarriedAmmo(CarriedAmmo); //Imprimimos por HUD la municion que tiene el character

		if (EquippedWeapon)
		{
			Controller->SetHUDWeaponType(EquippedWeapon->GetWeaponType());
		}
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartGrenadeLauncherAmmo);

}


/*
* Al poner "_Implementation" Estamos diciendo que es una funcion que utiliza multijugador
* En este caso es un metodo que se llama desde el servidor (ver UFUNCTION en .h)
*/
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("CombatComponent->ServerFire->HitTarget: (%d, %d, %d)"), TraceHitTarget.X, TraceHitTarget.Y, TraceHitTarget.Z);
	MulticastFire(TraceHitTarget);
}

/*
* Broadcast que se hace desde el servidor hacia a todas las maquinas
*/
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	//Solo entra al if si eres la maquina que controla el pawn localmente, por lo que no tienes que ejecutar este codigo
	//Ya que lo has ejecutado anteriormente en Fire()->LocalFire(). Primero se ejecuta en local para evitar notar el lag en los efectos del disparo, aunque lo notaras al matar a alguien
	//Ya que eso si está gestionado unicamante por el servidor.
	if (Character && Character->IsLocallyControlled()) return;

	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	MulticastShotgunFire(TraceHitTargets);
}

/*
* Broadcast del disparo de una escopeta
*/
void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	//Si eres la maquina controladora del character dueño de la escopeta, te saltas la funcion porque este codigo ya ha sido ejecutado en local
	if (Character && Character->IsLocallyControlled()) return;
	LocalShotgunFire(TraceHitTargets);
}

/*
* Funcion que ejecuta la logica de disparar en local para que la experiencia sea mas fluida
*/
void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;

	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (EquippedWeapon == nullptr) return;
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);

	if(Shotgun == nullptr || Character == nullptr) return;

	if (CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
	}
}

void UCombatComponent::DeathFireTimer()
{
	if (EquippedWeapon == nullptr)
	{
		FireTimer.Invalidate();
	}
	else
	{
		EquippedWeapon->Fire(HitTarget);
	}
}

/*
* Funcion que dispara el arma del jugador a tutiplen, se usa para cuando se quiera que el jugador muera disparando al aire
*/
void UCombatComponent::DeathFire()
{
	if (EquippedWeapon == nullptr) return;

	if (EquippedWeapon->bAutomatic)
	{

		Character->GetWorldTimerManager().SetTimer(
			FireTimer,									//Parametro que representa donde se guarda el timer
			this,										//Objeto que llama al timer
			&UCombatComponent::DeathFireTimer,			//Funcion que se ejecuta al terminar el timer
			EquippedWeapon->FireDelay,					//Tiempo del timer
			true
		);
	}else
	{
		EquippedWeapon->Fire(HitTarget);
	}
}

void UCombatComponent::EquipWeapon(AMasterWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		AFlag* Flag = Cast<AFlag>(WeaponToEquip);
		if (Flag)
		{
			EquipFlag(Flag);
		}
	}
	else
	{

		if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
		{
			EquipSecondaryWeapon(WeaponToEquip);
		}
		else
		{
			EquipPrimaryWeapon(WeaponToEquip);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

	}
}

/*
* Funcion que intercambia armas, principal con secundaria y viceversa
* Al sobreescribir una variable con otra, se guarda una cualquiera en una variable temporal para poder hacer el intercambio
*/
void UCombatComponent::SwapWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr) return;

	Character->PlaySwapMontage();
	Character->bFinishedSwapping = false;
	CombatState = ECombatState::ECS_SwappingWeapon; //Al cambiar el valor de CombatState se ejecuta OnRep_CombatState

	AMasterWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

}

void UCombatComponent::EquipPrimaryWeapon(AMasterWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	//Si ya tenemos un arma la tiramos
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(Character);	//Asignamos el character como dueño de la pistola
	EquippedWeapon->SetHUDAmmo();			//Imprimimos por HUD la municion del arma
	UpdateCarriedAmmo();					//Imprimimos por HUD la municion del character de ese arma
	PlayEquipWeaponSound(WeaponToEquip);
}

void UCombatComponent::EquipSecondaryWeapon(AMasterWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Secondary);
	AttachActorToBack(WeaponToEquip);
	SecondaryWeapon->SetOwner(Character);
	PlayEquipWeaponSound(WeaponToEquip);

	//Si el server(quien ejecuta esta funcion) no es el dueño, set del outline a false (lo tiene otro jugador en la espalda)
	if (!Character->IsLocallyControlled())
	{
		SecondaryWeapon->EnableCustomDepth(false);
	}
}

/*
* Funcion que se ejecuta cuando se recoge una bandera
*/
void UCombatComponent::EquipFlag(AFlag* FlagToEquip)
{
	if (Character && Character->GetTeam() != ETeam::ET_NoTeam && FlagToEquip == nullptr) return;

	if (FlagToEquip->GetTeam() == Character->GetTeam())
	{
		FlagToEquip->SetWeaponState(EWeaponState::EWS_Initial); //Al recoger una bandera de tu equipo, la devuelves a su base
																//(al cambiar EWeaponState en MasterWeapon.cpp se ejecuta OnRep_WeaponState)
	}
	else
	{
		FlagEquipped = FlagToEquip;
		AActor* Flag = Cast<AActor>(FlagToEquip);
		FlagToEquip->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToBack(Flag);
		FlagToEquip->SetOwner(Character);
	}
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}


void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && !bLocallyReloading)
	{
		ServerReload();	//Se ejecuta la recarga en el servidor para propagarlo en el resto de clientes
		HandleReload(); //Se ejecuta localmente para no esperar que el servidor lo propague al resto de clientes (mejora la experiencia si el que recarga tiene lag)
		bLocallyReloading = true;
	}
}

void UCombatComponent::PlayEquipWeaponSound(AMasterWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}

/*
* Function that attach the actor given by param to the back of the character using the socket "BackSocket"
*/
void UCombatComponent::AttachActorToBack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* BackSocket = Character->GetMesh()->GetSocketByName(FName("BackSocket"));
	if (BackSocket)
	{
		BackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

/*
* Function that attach the flag given by param to the back of the character using the socket "FlagSocket"
*/
void UCombatComponent::AttachFlagToBack(AActor* FlagToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || FlagToAttach == nullptr) return;
	
	const USkeletalMeshSocket* FlagSocket = Character->GetMesh()->GetSocketByName(FName("FlagSocket"));
	if (FlagSocket)
	{
		FlagSocket->AttachActor(FlagToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))	 //Comprobamos si en nuestro Map de municiones existe la Key del arma que tenemos
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];	//Si existe, obtenemos el valor de esa key para asignarsela a la municion del character
	}

	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);//Imprimimos por HUD la municion que tiene el character
		//Aunque esto se ejecute en el servidor OnRep_CarriedAmmo tiene el mismo codigo.
		
		Controller->SetHUDWeaponType(EquippedWeapon->GetWeaponType());	//Imprimimos por HUD el arma que tiene el character
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;

	//Municion faltante en el cargador del arma
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo(); 

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried); //El valor minimo entre las balas que tengo y las que faltan en el cargador
															//Son las que se meteran en el cargador al recargar

		//Este clamp esta por si la municion del arma es mayor que la del magcapacity (no tendria sentido pero si se programa mal puede pasar)
		//La resta de la variable RoomInMag saldria negativo y por tanto Least saldria negativo
		return FMath::Clamp(RoomInMag, 0, Least);	

	}
	
	//Si no entramos en el if significa que no existe ese tipo de arma en la variable CarriedAmmoMap
	return 0;
}

//Funcion que se ejecutara en el servidor
void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;				//Al cambiar CombatState se ejecuta OnRep_CombatState, por lo que hay codigo en esa funcion
															//para evitar que se ejecute HandleReload() de nuevo.

	if(!Character->IsLocallyControlled()) HandleReload();	//Si eres el servidor ya recargaste localmente
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	bLocallyReloading = false;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::FinishSwap()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	if (Character) Character->bFinishedSwapping = true;
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	//Logica de cambiar outlines antes de reasignar variables (Equipped->Seconday & Secondary->Equipped)
	if (Character->IsLocallyControlled())
	{
		SecondaryWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		SecondaryWeapon->EnableCustomDepth(true);

		EquippedWeapon->EnableCustomDepth(false);
	}

	//Pasos necesarios al pasar de Secondary a Equipped
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	//Pasos necesarios al pasar de Equipped a Secondary
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Secondary);
	AttachActorToBack(SecondaryWeapon);

}

//Funcion que se ejecuta en todos los clientes -->DOREPLIFETIME(...); cuando cambia la variable CombatState
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if(Character && !Character->IsLocallyControlled()) HandleReload(); //We already played it locally on Reload()
		break;

	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;

	case ECombatState::ECS_SwappingWeapon:
		if (Character && !Character->IsLocallyControlled()) //Ya recargamos localmente en ShooterCharacter.cpp
		{
			Character->PlaySwapMontage();
		}
		break;
	}

}

void UCombatComponent::UpdateAmmoValues()
{
	if (EquippedWeapon == nullptr)	return;

	int32 ReloadAmount = AmountToReload();

	//Logica para actualizar las variables de la municion que tiene el player por cuenta propia
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;	//Substraemos la cantidad que vamos a recargar
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];		//Actualizamos la variable de municion restante fuera del Map

	}


	//Actualizamos el HUD de CarriedAmmo ( El HUD del WeaponAmmo se actualiza en AddAmmo() )
	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{

		Controller->SetHUDCarriedAmmo(CarriedAmmo); //Imprimimos por HUD la municion que tiene el character
	}


	EquippedWeapon->AddAmmo(ReloadAmount);	//Añadimos la municion al arma

}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

ECombatState UCombatComponent::GetCombatState()
{
	return CombatState;
}