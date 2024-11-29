// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagGameMode.h"
#include "UdemyShooterOnline/Weapon/Flag.h"
#include "UdemyShooterOnline/CaptureTheFlag/FlagZone.h"
#include "UdemyShooterOnline/GameState/ShooterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminited(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	AShooterGameMode::PlayerEliminited(ElimmedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* FlagZone)
{
	bool bValidCapture = Flag->GetTeam() != FlagZone->Team;
	AShooterGameState* SGameState = Cast<AShooterGameState>(GameState);
	if (SGameState)
	{
		if (FlagZone->Team == ETeam::ET_BlueTeam)
		{
			SGameState->BlueTeamAddScore();
		}
		if (FlagZone->Team == ETeam::ET_RedTeam)
		{
			SGameState->RedTeamAddScore();
		}
	}
}
