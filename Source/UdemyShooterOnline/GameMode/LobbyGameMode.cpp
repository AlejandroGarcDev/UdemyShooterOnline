// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//Recogemos el numero de jugadores unidos en la sesion
	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	//Si ha llegado al numero de jugadores viajamos del lobby al mapa deseado
	if (NumberOfPlayers == 2)
	{
		//Recogemos el mundo para decirle que viaje
		UWorld* World = GetWorld();
		if (World)
		{
			//Utilizamos viaje seamless (cliente no se desconecta del servidor al cambiar de mapa)
			bUseSeamlessTravel = true;

			World->ServerTravel(FString("/Game/Maps/GameMap1?listen"));
		}
	}
}
