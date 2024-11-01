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

#define TRACE_LENGHT 80000.f

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
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); //Al poner _Condition, podemos poner un condicional en la replicacion,
																			//En este caso COND_OwnerOnly hace que se replique desde el servidor hasta el
																			//unico cliente posee el character de este combatcomponent
																			//Esto esta hecho así porque la municion de cada personaje no hace falta replicarlo
																			//A todos los clientes

	DOREPLIFETIME(UCombatComponent, CombatState);

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

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}


		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
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
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGHT;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		
		//Si implementa la interfaz InteractWithCrosshair se entiende que es un enemigo 
		//(al momento de escribir esto solo ShooterCharacter implementa esa interfaz)

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{

		HUDPackage.CrosshairColor = FLinearColor::Red;

		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}

		/*

		//Por como funciona el LineTrace, si no ha chocado con nada, no se rellena el impact point, el cual es un vector2d
		//Nosotros necesitamos esa informacion aunque no choque con nada para llevar los proyectiles (cohetes de un lanzacohetes), 
		//Asi que lo gestionamos manualmente
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}else
		{
			HitTarget = TraceHitResult.ImpactPoint;

		}

		*/

	}

}



/*
* Funcion que se ejecuta en el character que esté disparando, esta funcion llamara al servidor para que propague la funcion a todas las maquinas conectadas
*/
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;


	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}

}
void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;

		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = EquippedWeapon->GetCrosshairShootingSpread(); //Factor que desestabiliza la mira cuando se dispara, depende del propio arma
			CrosshairShootInterpSpeed = EquippedWeapon->GetCrosshairShootInterpSpeed(); //Velocidad con la que se reajusta el crosshair al disparar, depende del propio arma
		}
		StartFireTimer();
	}	
}


void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)	return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,									//Parametro que representa donde se guarda el timer
		this,										//Objeto que llama al timer
		&UCombatComponent::FireTimerFinished,		//Funcion que se ejecuta al terminar el timer
		EquippedWeapon->FireDelay);									//Tiempo del timer
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

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

}


/*
* Al poner "_Implementation" Estamos diciendo que es una funcion que utiliza multijugador
* En este caso es un metodo que se llama desde el servidor (ver UFUNCTION en .h)
*/
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

/*
* Broadcast que se hace desde el servidor hacia los clientes y el propio servidor
*/
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;

	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
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
	
	//Si ya tenemos un arma la tiramos
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);	//Asignamos el character como dueño de la pistola
	EquippedWeapon->SetHUDAmmo();			//Imprimimos por HUD la municion del arma



	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) //Comprobamos si en nuestro Map de municiones existe la Key del arma que tenemos
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];	//Si existe, obtenemos el valor de esa key para asignarsela a la municion del character
	}

	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{

		Controller->SetHUDCarriedAmmo(CarriedAmmo); //Imprimimos por HUD la municion que tiene el character
													//Aunque esto se ejecute en el servidor OnRep_CarriedAmmo tiene el mismo codigo.

		Controller->SetHUDWeaponType(EquippedWeapon->GetWeaponType());
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::HandleReload()
{

	Character->PlayReloadMontage();
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

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
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

//Funcion que se ejecuta en todos los clientes -->DOREPLIFETIME(...); cuando cambia la variable CombatState
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;

	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
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
