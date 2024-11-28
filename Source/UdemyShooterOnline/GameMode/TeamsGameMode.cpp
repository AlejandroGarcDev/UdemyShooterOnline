// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "UdemyShooterOnline/GameState/ShooterGameState.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"


ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

/*
* Si alguien entra a mitad de la partida, se le asigna un equipo
*/
void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AShooterGameState* BGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		AShooterPlayerState* ShooterPState = NewPlayer->GetPlayerState<AShooterPlayerState>();

		if (ShooterPState && ShooterPState->GetTeam() == ETeam::ET_NoTeam)
		{
			//Compruebo a que equipo se asigna cada jugador en funcion de la cantidad de cada equipo
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(ShooterPState);
				ShooterPState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BGameState->BlueTeam.AddUnique(ShooterPState);
				ShooterPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}

}

/*
* Si algun jugador sale de la partida, se le elimina del equipo
*/
void ATeamsGameMode::Logout(AController* Exiting)
{
	AShooterGameState* BGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	AShooterPlayerState* ShooterPState = Exiting->GetPlayerState<AShooterPlayerState>();
	if (BGameState && ShooterPState)
	{
		if (BGameState->RedTeam.Contains(ShooterPState))
		{
			BGameState->RedTeam.Remove(ShooterPState);
		}
		if (BGameState->BlueTeam.Contains(ShooterPState))
		{
			BGameState->BlueTeam.Remove(ShooterPState);
		}
	}

	Super::Logout(Exiting);
}

/*
* Funcion que gestiona la logica de evitar daño a compañeros
*/
float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	AShooterPlayerState* AttackerPlayerState = Attacker->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* VictimPlayerState = Victim->GetPlayerState<AShooterPlayerState>();

	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return BaseDamage;
	if (VictimPlayerState == AttackerPlayerState) return BaseDamage; //No se previene el daño a si mismo (se puede cambiar eso si se desea)
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		return 0.f; //Si son del mismo equipo el daño recibido es 0
	}
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminited(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	Super::PlayerEliminited(ElimmedCharacter, VictimController, AttackerController);

	AShooterGameState* SGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
	if (SGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			SGameState->BlueTeamAddScore();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			SGameState->RedTeamAddScore();
		}
	}
}

/*
* Cuando la partida por equipos empieza, se asignan los jugadores a cada equipo
*/
void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	AShooterGameState* BGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		//Recorremos todos los jugadores
		for (auto PState : BGameState->PlayerArray)
		{
			AShooterPlayerState* ShooterPState = Cast<AShooterPlayerState>(PState.Get());
			if (ShooterPState && ShooterPState->GetTeam() == ETeam::ET_NoTeam)
			{
				//Compruebo a que equipo se asigna cada jugador en funcion de la cantidad de cada equipo
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(ShooterPState);
					ShooterPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(ShooterPState);
					ShooterPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}

}
