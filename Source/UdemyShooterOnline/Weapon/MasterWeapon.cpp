// Fill out your copyright notice in the Description page of Project Settings.


#include "MasterWeapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "UdemyShooterOnline/Weapon/Flag.h"

// Sets default values
AMasterWeapon::AMasterWeapon() 
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh -> SetupAttachment(RootComponent);

	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE); //CUSTOM... defined on WeaponTypes
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);

	//El AreaSphere nos servirá para cuando estemos en ese area que nos salte un widget para recoger el arma
	//Sin embargo, para un juego multijugador es necesario que este proceso suceda en el servidor para que se realice de manera segura
	//Por ello, seteamos las colisiones como ignorar ya que todo lo que no sea el servidor ignorara el AreaSphere
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AMasterWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}

}

void AMasterWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	//HasAuthority hace la siguiente expresion: GetLocalRole() == ENetRole::ROLE_Authority
	//Esto quiere decir que comprueba si en local tiene el rol Authority, esto quiere decir que solo el servidor podrá tener las fisicas activadas
	//Por supuesto el cliente vera un reflejo de la interaccion pero será el servidor el encargado de manejar las colisiones
	//Para tener mas claro los roles podemos ir a OverweadWidget y poner Local en lugar de Remote y ver que sucede si
	//Simulamos con 3 jugadores, siendo 1 el servidor. Veremos que en la pantalla del servidor los 3 personajes tendran authority.

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

		//Bindeas el overlap con la funcion Overlap creada en esta clase
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);

		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}
}

void AMasterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMasterWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMasterWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AMasterWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

/*
* Esta funcion se ejecuta cuando la variable Owner es cambiada en el servidor, de esta manera el cliente puede tener la variable actualizada pero es el servidor quien lo cambia
*/
void AMasterWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	//Si seteamos Owner a null en el servidor (Funcion Dropped()) tenemos que propagarlo al resto de clientes tambien
	if (Owner == nullptr)
	{
		ShooterOwnerCharacter = nullptr;
		ShooterOwnerController = nullptr;
	}
	else
	{
		//Aqui comprobamos si al cambiar el dueño del arma, dicho dueño lo tiene como arma principal (y no secundaria o no equipada)
		//Si es así, se actualiza la municion del arma principal en el HUD
		ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(Owner) : ShooterOwnerCharacter;
		if (ShooterOwnerCharacter && ShooterOwnerCharacter->GetEquippedWeapon() && ShooterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
		
	}
}

void AMasterWeapon::SetHUDAmmo()
{
	ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterOwnerCharacter;
	if (ShooterOwnerCharacter)
	{
		ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(ShooterOwnerCharacter->Controller) : ShooterOwnerController;

		if (ShooterOwnerController)
		{
			ShooterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AMasterWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		//Comprobamos si es una bandera, si lo es la logica en la siguiente:
		//Estado inicial->Bandera en la base == Solo los del equipo enemigo pueden cogerla
		//Estado equipped->En la espalda de un enemigo == No la puede coger nadie (colisiones desactivadas asi que esta opcion nunca aparecera)
		//Estado dropped->Esta en el suelo (tras matar al portador) == Todos pueden cogerla, pero dependiendo de que equipo lo haga su funcion cambia (CombatComponent->EquipWeapon)
		if (WeaponType == EWeaponType::EWT_Flag)
		{
			if (WeaponState == EWeaponState::EWS_Initial && ShooterCharacter->GetTeam() != Team)
			{
				ShooterCharacter->SetOverlappingWeapon(this);
			}
			else if (WeaponState == EWeaponState::EWS_Dropped)
			{
				ShooterCharacter->SetOverlappingWeapon(this);
			}
		}
		else
		{
			ShooterCharacter->SetOverlappingWeapon(this);
		}
	}
}

void AMasterWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && ShooterCharacter->GetTeam() != Team) return;
		ShooterCharacter->SetOverlappingWeapon(nullptr);
	}
}

/*
* Funcion que gestiona la logica cuando se dispara con el arma: 1) Restar una bala; 2) Actualizar HUD
* 
* Teniendo en cuenta que esto se llama desde el servidor [AMasterWeapon::Fire()], se ejecuta la misma logica en OnRep_Ammo
*/
void AMasterWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	
	//Server reconciliation
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else if (ShooterOwnerCharacter && ShooterOwnerCharacter->IsLocallyControlled())
	{
		++Sequence;
	}
}

/*
* Funcion llamada desde el server que actualiza la municion del cliente,
* utiliza una variable que define cuantos disparos están sin procesar todavia, por lo que tiene esta variable en cuenta para marcar las balas que quedan
* aunque no esten todavia replicadas al cliente
*/
void AMasterWeapon::ClientUpdateAmmo_Implementation(uint32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;

	Ammo -= Sequence;	//Restamos a la municion las iteraciones de disparo que todavia no se han replicado (debido al lag)
						//Esta linea de codigo sirve siempre y cuando al disparar solo gastes una bala
	SetHUDAmmo();
}

void AMasterWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AMasterWeapon::ClientAddAmmo_Implementation(uint32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

/*
* Funcion que cambia el estado de un arma
*/
void AMasterWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;

	OnWeaponStateSet();
}

void AMasterWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

/*
* Funcion que se ejecuta en todos los clientes (no hay _cond en DOREPLIFETIME) cuando se cambia la variable WeaponState
*/
void AMasterWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

/*
* Funcion que ejecuta segun que funcion dependiendo del nuevo estado del arma
*/
void AMasterWeapon::OnWeaponStateSet()
{

	switch (WeaponState)
	{

	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	
	case EWeaponState::EWS_Secondary:
		OnEquippedSecondary();
		break;

	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	case EWeaponState::EWS_Initial:
		AFlag* Flag = Cast<AFlag>(this);
		if (Flag)
		{
			Flag->SetActorLocation(Flag->InitialPosition);
			Flag->SetActorRotation(FQuat(FRotator(0.f, 0.f, 0.f)));
			Flag->SetOwner(nullptr);
			FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
			Flag->GetWeaponMesh()->DetachFromComponent(DetachRules);
			Flag->SetOwner(nullptr);
			ShooterOwnerCharacter = nullptr;
			ShooterOwnerController = nullptr;
		}
	}

}

/*
* Funcion que ejecuta la logica cuando un arma pasa a ser equipada
* 1) Deshabilita colissiones
* 2) Desactiva gravedad y fisicas (a excepcion del subfusil ya que tiene una banda que tiene PhysicMaterial integrado)
*/
void AMasterWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		WeaponMesh->WakeAllRigidBodies();
	}
	EnableCustomDepth(false);

	ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterOwnerCharacter;
	if (ShooterOwnerCharacter && bUseServerSideRewind)
	{
		ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(ShooterOwnerCharacter->Controller) : ShooterOwnerController;
		if (ShooterOwnerController && HasAuthority() && !ShooterOwnerController->HighPingDelegate.IsBound()) //Comprobamos si no hay funcion unida al delegate
		{
			ShooterOwnerController->HighPingDelegate.AddDynamic(this, &AMasterWeapon::OnPingTooHigh);
		}
	}
}

/*
* Funcion que ejecuta la logica cuando un arma pasa a ser arma secundaria
* 1) Deshabilita colissiones
* 2) Desactiva gravedad y fisicas (a excepcion del subfusil ya que tiene una banda que tiene PhysicMaterial integrado)
* 3) Activa outline blanco
*/
void AMasterWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		WeaponMesh->WakeAllRigidBodies();
	}
	/*
	//Comprobamos si es el servidor quien recoge el arma, si es asi, se activa directamente en el servidor (esta funcion se llama desde el servidor)
	if (ShooterOwnerCharacter->IsLocallyControlled())
	{
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}
	else
	{
		EnableCustomDepth(false);
	}
	*/

	ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterOwnerCharacter;
	if (ShooterOwnerCharacter && bUseServerSideRewind)
	{
		ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(ShooterOwnerCharacter->Controller) : ShooterOwnerController;
		if (ShooterOwnerController && HasAuthority() && ShooterOwnerController->HighPingDelegate.IsBound()) //Comprobamos si no hay funcion unida al delegate
		{
			ShooterOwnerController->HighPingDelegate.RemoveDynamic(this, &AMasterWeapon::OnPingTooHigh);
		}
	}

}


/*
* Funcion que ejecuta la logica cuando un arma pasa a dropped (Ver OnWeaponState)
* 1) Habilita colisiones
* 2) Activa gravedad y fisicas
* 3) Desactiva colisiones con pawn y camera (para que no se trabe al soltar el arma con ninguno de los dos)
* 4) Activa el outline
*/
void AMasterWeapon::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);	//Nota: En equipped no lo ponemos como falso, porque puede estar equipado pero a la espalda, en cuyo caso se pone un
									//Outline blanco. Por lo tanto se ha decidido setear a falso el CustomDepth en combat component, cuando equipamos un arma como principal
	}		

	ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterOwnerCharacter;
	if (ShooterOwnerCharacter && bUseServerSideRewind)
	{
		ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(ShooterOwnerCharacter->Controller) : ShooterOwnerController;
		if (ShooterOwnerController && HasAuthority() && ShooterOwnerController->HighPingDelegate.IsBound()) //Comprobamos si no hay funcion unida al delegate
		{
			ShooterOwnerController->HighPingDelegate.RemoveDynamic(this, &AMasterWeapon::OnPingTooHigh);
		}
	}

}


bool AMasterWeapon::IsEmpty()
{
	return Ammo <= 0;
}

void AMasterWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

/*
* Funcion que se llamara cuando se dispare en el personaje
*/
void AMasterWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation,false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")); //Socket del arma desde donde salen los casquillos

		if (AmmoEjectSocket)
		{

			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			
			UWorld* World = GetWorld();

			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
	SpendRound();
}

/*
* Funcion que se ejecuta en el servidor al tirar un arma
*/
void AMasterWeapon::Dropped()
{
	//EWeaponState es una variable replicada, eso quiere decir que si la llamamos desde el servidor se propaga al resto de clientes de forma automatica
	SetWeaponState(EWeaponState::EWS_Dropped);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);

	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	ShooterOwnerCharacter = nullptr;
	ShooterOwnerController = nullptr;
}


/*
* Al momento de disparar un arma con dispersion (escopeta) se ejecuta esta funcion:
* 1) Se genera una esfera a la distancia "DistanceToSphere" que servira como referencia para tomar puntos aleatorios de ella y generar los perdigones
* 2) Se genera un vector aleatorio desde el centro del origen hasta, como maximo, el perimetro de la esfera
* 3) Teniendo el punto aleatorio generado se crea la trayectoria del perdigon
*/
FVector AMasterWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	//Se normaliza la distancia a la que apunta para despues generar una esfera de referencia a la hora de generar trayectorias aleatorias
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	//Genero la localizacion random dentro de la esfera
	const FVector RandomVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);

	//Añado offset o posicion de origen de la esfera al punto aleatorio generado
	const FVector EndLoc = SphereCenter + RandomVec;

	//Genero el vector de desplazamiento de la bala
	const FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGHT / ToEndLoc.Size());
}


