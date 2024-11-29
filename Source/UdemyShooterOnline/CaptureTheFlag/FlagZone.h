// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UdemyShooterOnline/ShooterTypes/Team.h"
#include "FlagZone.generated.h"

UCLASS()
class UDEMYSHOOTERONLINE_API AFlagZone : public AActor
{
	GENERATED_BODY()
	
public:	

	AFlagZone();

	UPROPERTY(EditAnywhere)
	ETeam Team;
protected:

	virtual void BeginPlay() override;

	//Esta funcion es la que nos indicara cuando el jugador esta dentro del area
	//Los parametros de entrada vienen dados por el padre AActor
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:

	UPROPERTY(EditAnywhere)
	class USphereComponent* ZoneSphere;
public:	

};
