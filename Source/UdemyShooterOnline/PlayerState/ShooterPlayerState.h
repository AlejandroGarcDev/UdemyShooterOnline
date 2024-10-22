// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public: 

	virtual void GetLifetimeReplicatedProps(TArray <FLifetimeProperty> &OutLifetimeProps) const override;

	/*
	* Sabiendo que PlayerState es una clase dedicada a guardar la info de un jugador durante la partida,
	* Unreal tiene implementado varias funcionalidades a APlayerState
	* Una de ellas es tener una variable "Score" con replicacion activada
	* Utilizaremos esta variable para guardar la puntuacion de cada player.
	*/
	virtual void OnRep_Score() override;

	/*
	* notify function para replicar Defeats
	* 
	* Recordatorio: OnRep_ Tiene que ser UFUNCTION() (OnRep_Score es UFUNCTION pero no lo ponemos porque es override)
	*/
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);

	void AddToDefeats(int32 DefeatsAmount);

private:

	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	//Otra forma es inicializarlo como nullptr => class AShooterCharacter* Character = nullptr 
	UPROPERTY()
	class AShooterCharacter* Character;


	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	UPROPERTY()
	class AShooterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
