// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole RemoteRole = InPawn->GetRemoteRole();		//Rol del pawn a nivel multijugador
	ENetRole LocalRole = InPawn->GetLocalRole();		//Rol del pawn a nivel local
	FString Role;
	switch (LocalRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");			//Servidor
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");		//El jugador tiene control sobre este pawn
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");		//El jugador no tiene control sobre este pawn (otro player)
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	FString RemoteRoleString = FString::Printf(TEXT("Local Role: %s"),*Role);
	RemoteRoleString.Append(ShowPlayerName(InPawn->GetPlayerState()));
	
	//SetDisplayText(RemoteRoleString); Poner esta linea si se quiere saber el rol de cada jugador a en cada maquina
}

FString UOverheadWidget::ShowPlayerName(APlayerState* PS)
{	
	if (PS)
	{
		FString PlayerName = PS->GetPlayerName();
		return PlayerName;
	}
	else
	{
		return FString("");
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();

}
