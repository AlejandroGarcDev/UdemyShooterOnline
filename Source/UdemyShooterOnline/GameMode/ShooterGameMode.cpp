// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameMode.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"
#include "UdemyShooterOnline/GameState/ShooterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AShooterGameMode::AShooterGameMode()
{
	bDelayedStart = true;
}


void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AShooterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	//Recorre todos los controladores que existan
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AShooterPlayerController* ShooterPlayer = Cast<AShooterPlayerController>(*It);
		if (ShooterPlayer)
		{
			ShooterPlayer->OnMatchStateSet(MatchState);
		}
	}
}


void AShooterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (MatchState == MatchState::WaitingToStart)
	{
		//LevelStartingTime es el tiempo que ha pasado hasta que entras en el mundo (menu, opciones, lo que sea)
		//Se lo restas a GetWorld->GetTimeSeconds y tienes el tiempo que ha pasado desde que has entrado al mundo
		//Una vez tienes ese tiempo se lo vas restando a TiempoDeEspera (WarmupTime). Cuando alcance 0 ya puedes empezar la partida
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}


void AShooterGameMode::PlayerEliminited(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
	AShooterPlayerState* VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;

	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && GameState)
	{
		TArray<AShooterPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : ShooterGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.0f);
		ShooterGameState->UpdateTopScore(AttackerPlayerState); //Actualizamos regristro del ganador de la partida

		if (ShooterGameState->TopScoringPlayers.Contains(AttackerPlayerState)) //Si el atacante entra en TopScorinPlayer, le damos la corona
		{
			AShooterCharacter* Leader = Cast<AShooterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++) //Comprobamos si hay algun player que haya perdido el lider (al haber otro player que sume 1 punto, el que iba primero puede haber pasado a segundo)
		{
			if (!ShooterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				AShooterCharacter* Loser = Cast<AShooterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}

	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}

	//Accedemos a todos los controladores para que cada uno muestre por pantalla que un jugador a matado a otro jugador
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		AShooterPlayerController* ShooterPlayer = Cast<AShooterPlayerController>(*It);
		if (ShooterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			ShooterPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void AShooterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset(); //Dejamos de poseer el pawn que tengamos, de esta forma podemos poseer un nuevo pawn, character, etc.


		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		TArray<AActor*> PlayerStart;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStart);
		int32 Selection = FMath::RandRange(0, PlayerStart.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStart[Selection]);
	}
}


void AShooterGameMode::PlayerLeftGame(AShooterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (ShooterGameState && ShooterGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		ShooterGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	AShooterCharacter* CharacterLeaving = Cast<AShooterCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}
