// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameState.h"
#include "Net/UnrealNetwork.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState, TopScoringPlayers);
	DOREPLIFETIME(AShooterGameState, RedTeamScore);
	DOREPLIFETIME(AShooterGameState, BlueTeamScore);
}

void AShooterGameState::UpdateTopScore(class AShooterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)				//Si no hay nadie se agrega sin comprobar nada
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore) //Alguien iguala a el que iba 1�
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore) //Alguien supera a el que iba 1�
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void AShooterGameState::RedTeamAddScore()
{
	++RedTeamScore;
	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void AShooterGameState::BlueTeamAddScore()
{
	++BlueTeamScore;
	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void AShooterGameState::OnRep_RedTeamScore()
{
	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void AShooterGameState::OnRep_BlueTeamScore()
{
	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
