// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerState.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Net/UnrealNetwork.h"





/*
* Funcion que sirve para replicar variables, esta inherente en PlayerState.cpp ya que tiene variables como la puntuacion, PlayerName, PlayerId... 
* que usan replicacion
*/
void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerState, Defeats);
	DOREPLIFETIME(AShooterPlayerState, Team);

}


void AShooterPlayerState::OnRep_Defeats()
{

}

/*
* Funcion de la logica de añadir puntuacion
* 
* Se llama tanto en el servidor como en los clientes:
* Servidor:
* Clientes: OnRep_Score
*/
void AShooterPlayerState::AddToScore(float ScoreAmount)
{

	SetScore(GetScore() + ScoreAmount); //Funcion inherente de PlayerState

	Character = Character == nullptr ? Cast<AShooterCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;

		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void AShooterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;

	Character = Character == nullptr ? Cast<AShooterCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;

		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

/*
* Funcion que se llama desde el servidor a los clientes (se ejecutan en los clientes)
*/
void AShooterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character == nullptr ? Cast<AShooterCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;

		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

/*
* Funcion que se ejecuta en todas las maquinas al cambiar el valor del equipo
* La funcion asigna el color del jugador en funcion del nuevo equipo
*/
void AShooterPlayerState::OnRep_Team()
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
	if (ShooterCharacter)
	{
		ShooterCharacter->SetTeamColor(Team);
	}
}

/*
* Ademas de asignar el nuevo equipo, llama al character y le asigna su nuevo color de la malla (EquipoAzul->ColorAzul en el Character)
*/
void AShooterPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
	if (ShooterCharacter)
	{
		ShooterCharacter->SetTeamColor(Team);
	}
}
