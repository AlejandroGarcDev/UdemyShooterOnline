// Fill out your copyright notice in the Description page of Project Settings.


#include "Flag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	SetRootComponent(FlagMesh);
	GetAreaSphere()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);

	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/*
* Funcion que se ejecuta desde el servidor al tirar un arma
*/
void AFlag::Dropped()
{
	//EWeaponState es una variable replicada, eso quiere decir que si la llamamos desde el servidor se propaga al resto de clientes de forma automatica
	SetWeaponState(EWeaponState::EWS_Dropped);

	if (FlagMesh)
	{
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		FlagMesh->DetachFromComponent(DetachRules);
	}

	SetOwner(nullptr);
	ShooterOwnerCharacter = nullptr;
	ShooterOwnerController = nullptr;

	//Al soltar una bandera se pone en el suelo de pie, para que pueda ser vista facilmente, para ello lanzamos un rayo desde la posicion donde está + un margen de seguridad
	FVector InicioLaneTrace = GetActorLocation();
	FVector FinalLaneTrace = InicioLaneTrace + FVector(0.f, 0.f, -25.f);
	InicioLaneTrace += FVector(0.f, 0.f, +30.f);
	FHitResult HandleResult;

	GetWorld()->LineTraceSingleByChannel(
		HandleResult,
		InicioLaneTrace,
		FinalLaneTrace,
		ECollisionChannel::ECC_Visibility
	);

	if(HandleResult.bBlockingHit)
	{
		SetActorLocation(HandleResult.ImpactPoint + FVector(0.f, 0.f, +10.f));
		SetActorRotation(FQuat(FRotator(0.f, 0.f, GetActorRotation().Yaw)));
	}
}

void AFlag::OnEquipped()
{
	ShowPickupWidget(false);
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnableCustomDepth(false);
}

/*
* Funcion que se ejecuta en todos los clientes al cambiar el estado del arma WeaponState Dropped
*/
void AFlag::OnDropped()
{
	if (HasAuthority())
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (FlagMesh)
	{
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	}
	//Al soltar una bandera se pone en el suelo de pie, para que pueda ser vista facilmente, para ello lanzamos un rayo desde la posicion donde está + un margen de seguridad
	FVector InicioLaneTrace = GetActorLocation();
	FVector FinalLaneTrace = InicioLaneTrace + FVector(0.f, 0.f, -25.f);
	InicioLaneTrace += FVector(0.f, 0.f, +30.f);
	FHitResult HandleResult;

	GetWorld()->LineTraceSingleByChannel(
		HandleResult,
		InicioLaneTrace,
		FinalLaneTrace,
		ECollisionChannel::ECC_Visibility
	);

	if (HandleResult.bBlockingHit)
	{
		SetActorLocation(HandleResult.ImpactPoint + FVector(0.f,0.f,+10.f));
		SetActorRotation(FQuat(FRotator(0.f, 0.f, GetActorRotation().Yaw)));
	}
}
