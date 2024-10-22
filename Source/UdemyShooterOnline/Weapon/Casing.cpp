// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
ACasing::ACasing()
{

	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 5.f;
}


void ACasing::BeginPlay()
{
	Super::BeginPlay();

	//Esto hace que cuando ocurra un hit se ejecute ACasing::OnHit
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);	

	CasingMesh->AddImpulse(GetActorForwardVector()*ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	UWorld* World = GetWorld();
	if (World)
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::DestroyFunc, 3.0f, false);
	}
}

void ACasing::DestroyFunc()
{
	Destroy();
}
