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

// Sets default values
AMasterWeapon::AMasterWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh -> SetupAttachment(RootComponent);

	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	DOREPLIFETIME(AMasterWeapon, Ammo);
}

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
		SetHUDAmmo();
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
		ShooterCharacter->SetOverlappingWeapon(this);
	}
}

void AMasterWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AMasterWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{

	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		break;
	}
}

/*
* Funcion que gestiona la logica cuando se dispara con el arma: 1) Restar una bala; 2) Actualizar HUD
* 
* Teniendo en cuenta que esto se llama desde el servidor [AMasterWeapon::Fire()] se ejecuta la misma logica en OnRep_Ammo
*/

void AMasterWeapon::SpendRound()
{
	--Ammo;
	SetHUDAmmo();
}

void AMasterWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}


void AMasterWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;

	switch (WeaponState)
	{

	case EWeaponState::EWS_Equipped:

		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		break;
	}
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

