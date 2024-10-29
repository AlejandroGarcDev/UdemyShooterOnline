// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"
#include "UdemyShooterOnline/HUD/ShooterHUD.h"
#include "UdemyShooterOnline/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "Net/UnrealNetwork.h"
#include "UdemyShooterOnline/GameMode/ShooterGameMode.h"
#include "UdemyShooterOnline/HUD/Announcement.h"



void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShooterHUD=GetHUD<AShooterHUD>();

	if (ShooterHUD)
	{
		ShooterHUD->AddAnnouncement();
	}
}


/*
* Esta funcion se llama cuando el controlador posee un personaje por primera vez, En ShooterCharacter.cpp inicializamos la vida y el HUD, pero,
* Al ser los primeros instantes del juego es posible que no este todo cargado y haya algun null pointer en algun elemento del HUD,
* Al sobreescribir esta funcion nos aseguramos que cuando el controlador cargue
* esta funcion se ejecute, y en esta volvemos a actualizar el HUD de la vida por segunda vez y no haya ningun error de que no haya 
* algun elemento todavia sin tener el valor correcto porque existio despues de que se inicializace
*/
void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn);
	
	if (ShooterCharacter)
	{
		SetHUDHealth(ShooterCharacter->GetHealth(), ShooterCharacter->GetMaxHealth());
	}

}

void AShooterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, MatchState);

}

void AShooterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;
	
	if (ShooterHUD &&								//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&				//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->HealthBar &&  //Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
		ShooterHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		ShooterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		ShooterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}

}

void AShooterPlayerController::SetHUDScore(float Score)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe ScoreAmount
	if (ShooterHUD &&								//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&				//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->ScoreAmount)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		ShooterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}

}

void AShooterPlayerController::SetHUDDefeats(int32 Defeats)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe DefeatsAmount
	if (ShooterHUD &&									//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&					//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->DefeatsAmount)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		ShooterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AShooterPlayerController::SetHUDWeaponAmmo(int32 WeaponAmmo)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe WeaponAmmoAmount
	if (ShooterHUD &&									//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&					//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->WeaponAmmoAmount)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		FString WeaponAmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
		ShooterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
	}

}

void AShooterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe CarriedAmmoAmount
	if (ShooterHUD &&										//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&						//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->CarriedAmmoAmount)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		ShooterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void AShooterPlayerController::SetHUDWeaponType(EWeaponType WeaponEquipped)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe WeaponEquipped
	if (ShooterHUD &&										//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&						//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->WeaponEquipped)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{

		FText SWeaponEquipped = UEnum::GetDisplayValueAsText<EWeaponType>(WeaponEquipped);
		ShooterHUD->CharacterOverlay->WeaponEquipped->SetText(SWeaponEquipped);
	}
}

void AShooterPlayerController::SetHUDMatchCountdown(float Time)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe MatchCountdownText
	if (ShooterHUD &&										//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&						//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->MatchCountdownText)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;

		//%02d imprime una variable integer con un formato de 2 digitos, si por ejemplo paso un 5 se imprimirá 05
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AShooterPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());

	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(SecondsLeft);
	}
	CountdownInt = SecondsLeft;
}

/*
* Funcion que comprueba que exista el HUD del personaje, si existe y la variable Initialize=true, significa que el HUD necesita ser actualizado mediante esta funcion
* Esto porque al momento de crearse el HUD la partida todavia no ha comenzado y no se asignan los valores iniciales
*/
void AShooterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (ShooterHUD && ShooterHUD->CharacterOverlay)
		{
			CharacterOverlay = ShooterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

/*
* Funcion que actualiza el delay que hay entre servidor y cliente
*/
void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;

	//Si sobrepasa el tiempo para volver a pedir el delay, se llama al servidor para que pase el tiempo
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());

	}
}

/*
* Esta funcion es ejecutada desde el servidor y lo que hace es pasar al cliente el momento de la peticion y el tiempo del mundo en el servidor
* Con esos datos el cliente puede saber cuanto tiempo ha pasado desde que se hizo la peticion hasta que obtuvo respuesta y calcular del delay total
* El delay total es el camino Cliente->Servidor y Servidor-Cliente. si dividimos el delay total tenemos una medida aprox. del delay entre Serv->Cliente
*/
void AShooterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

/*
* Funcion que calcula el delay entre Serv->Cliente segun los datos pasados por el servidor
*/
void AShooterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	//Calculas la diferencia entre el momento actual con el momento en el que hiciste la peticion para saber el delay
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;

	//Teniendo que el delay generado es por el trayecto Cliente->Servidor y Servidor->Cliente, si divides ese delay por la mitad y se lo sumas al tiempo
	//Que te ha dado el servidor, tendrás, aproximadamente, el tiempo que tiene el servidor al momento de leer esta linea de codigo
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime); 

	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

/*
* Funcion que devuelve el tiempo del mundo añadiendo el delay entre servidor y cliente
*/
float AShooterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

/*
* Funcion que se ejecuta al iniciarse el controlador, se ha sobreescrito esta funcion para sincronizar tiempos entre servidor y cliente
*/
void AShooterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

/*
* Funcion que se ejecuta en el servidor (se llama en GameMode --> Servidor)
* Indica el estado de la partida (iniciado, esperando para jugar, jugando, finalizado, abortado)
*/
void AShooterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	//Comprueba si la partida ha empezado y añade por pantalla el HUD el personaje
	if (MatchState == MatchState::InProgress)
	{
		ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;
		
		if (ShooterHUD)
		{
			ShooterHUD->AddCharacterOverlay();


			//Se comprueba si announcement (tiempo para que empiece la partida) está generado, si lo está, se hace invisible debido a que ya empezó la partida
			//No se borra porque al final de la partida se volvera a usar el widget (para no estar creando y borrando)
			if (ShooterHUD->Announcement)
			{
				ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

/*
* notify replication function para que se el cambio en MatchState se vea en los clientes
*/
void AShooterPlayerController::OnRep_MatchState()
{
	//Comprueba si la partida ha empezado y añade por pantalla el HUD el personaje
	if (MatchState == MatchState::InProgress)
	{
		ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

		if (ShooterHUD)
		{
			ShooterHUD->AddCharacterOverlay();

			//Se comprueba si announcement (tiempo para que empiece la partida) está generado, si lo está, se hace invisible debido a que ya empezó la partida
			//No se borra porque al final de la partida se volvera a usar el widget (para no estar creando y borrando)
			if (ShooterHUD->Announcement)
			{
				ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}