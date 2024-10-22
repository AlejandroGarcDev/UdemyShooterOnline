// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameMode.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"

void AShooterGameMode::PlayerEliminited(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
	AShooterPlayerState* VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
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
