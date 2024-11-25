// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"
#include "UdemyShooterOnline/HUD/ShooterHUD.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "UdemyShooterOnline/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "Net/UnrealNetwork.h"
#include "UdemyShooterOnline/GameMode/ShooterGameMode.h"
#include "UdemyShooterOnline/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "UdemyShooterOnline/ShooterComponents/CombatComponent.h"
#include "UdemyShooterOnline/GameState/ShooterGameState.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"
#include "Components/Image.h"
#include "UdemyShooterOnline/HUD/ReturnToMainMenu.h"


void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShooterHUD=GetHUD<AShooterHUD>();

	ServerCheckMatchState();
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
	CheckPing(DeltaTime);

}

/*
* Funcion que ejecuta toda la logica de avisar si hay mala conexion
*/
void AShooterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			//Se multiplica por 4 porque GetPing() comprime el ping dividiendolo por 4
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}

		HighPingRunningTime = 0.f;
	}

	bool bHighAnimationPlaying = ShooterHUD &&
		ShooterHUD->CharacterOverlay &&
		ShooterHUD->CharacterOverlay->HighPingAnimation
		&& ShooterHUD->CharacterOverlay->IsAnimationPlaying(ShooterHUD->CharacterOverlay->HighPingAnimation);

	if (bHighAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;

		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void AShooterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenutSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

//Is the ping too high?
void AShooterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, MatchState);

}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;
	if (UEnhancedInputComponent* EnhacedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhacedInputComponent->BindAction(QuitAction, ETriggerEvent::Completed, this, &AShooterPlayerController::ShowReturnToMainMenu);
	}
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

		bInitializeHealth = false;
		HUDHealth = 0.f;
		HUDMaxHealth = 0.f;

	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}

}

void AShooterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	if (ShooterHUD &&								//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&				//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->ShieldBar &&  //Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
		ShooterHUD->CharacterOverlay->ShieldText)
	{
		const float ShieldPercent = Shield / MaxShield;

		ShooterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		ShooterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));

		bInitializeShield = false;
		HUDShield = 0.f;
		HUDMaxShield = 0.f;
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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

		bInitializeScore = false;
		HUDScore = 0.f;
	}
	else
	{
		bInitializeScore = true;
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

		bInitializeDefeats = false;
		HUDDefeats = 0.f;

	}
	else
	{
		bInitializeDefeats = true;
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

		bInitializeWeaponAmmo = false;
		HUDWeaponAmmo = 0.f;
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = WeaponAmmo;

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

		bInitializeCarriedAmmo = false;
		HUDCarriedAmmo = 0.f;

	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmo;
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

/*
* Funcion que actualiza el tiempo del contador del tiempo de la partida
*/
void AShooterPlayerController::SetHUDMatchCountdown(float Time)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe MatchCountdownText
	if (ShooterHUD &&										//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&						//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->MatchCountdownText)	//Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
	{
		//Durante la transicion de estados (MatchState) hay un pequeño momento que el tiempo puede verse negativo
		if (Time < 0.f)
		{
			ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;

		//%02d imprime una variable integer con un formato de 2 digitos, si por ejemplo paso un 5 se imprimirá 05
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}


/*
* Funcion que actualiza el tiempo del contador del calentamiento (antes de la partida)
*/
void AShooterPlayerController::SetHUDAnnouncementCountdown(float Time)
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	//Comprobamos que existe WarmupTime
	if (ShooterHUD &&										//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->Announcement &&							//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->Announcement->WarmupTime)				//Comprobamos la variables dentro de Announcement. Alterar este orden puede provocar nullptr except.
	{
		//Durante la transicion de estados (MatchState) hay un pequeño momento que el tiempo puede verse negativo
		if (Time < 0.f)
		{
			ShooterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;

		//%02d imprime una variable integer con un formato de 2 digitos, si por ejemplo paso un 5 se imprimirá 05
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ShooterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}


void AShooterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	
	//WarmupTime == tiempo que se ha decidido que tiene que pasar hasta que empiece la partida
	//GetServerTime() == Da el tiempo que debera tener aprox el server (Se le suma una cantidad de delay)
	//LevelStartingTime == variable que se inicializa al momento de entrar en partida con GetTimeSeconds, por lo que es el tiempo que ha pasado hasta que ha comenzado la partida (en menus, partidas anteriores, etc...)
	//Si restamos a GetServerTime LevelStartingTime, tenemos el tiempo que ha transcurrido desde que se ha entrado a la partida
	//Si esa resta se la vamos restando a WarmupTime tenemos el tiempo restante para que la partida pase de fase (In progress)
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		ShooterGameMode = ShooterGameMode == nullptr ? Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)) : ShooterGameMode;
		if (ShooterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(ShooterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}

	}
	CountdownInt = SecondsLeft;
}

/*
* Funcion que registra valores iniciales de las variables del HUD (Vida, Puntuacion...)
* Esto porque al momento de inicializar el HUD los valores que tienen las variables del HUD no corresponden con los que deberia de tener
* Info: Esto se podria hacer en el HUD del personaje, pero debido a que el controlador es la interfaz entre el Pawn y el jugador tiene sentido hacerlo en el controller.
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
				if(bInitializeHealth)
					SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)
					SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore)
					SetHUDScore(HUDScore);
				if (bInitializeDefeats)
					SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo)
					SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo)
					SetHUDWeaponAmmo(HUDWeaponAmmo);
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
* Funcion que activa aviso de mala conexion
*/
void AShooterPlayerController::HighPingWarning()
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	if (ShooterHUD &&									//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&					//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->HighPingImage &&  //Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
		ShooterHUD->CharacterOverlay->HighPingAnimation)
	{
		ShooterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		ShooterHUD->CharacterOverlay->PlayAnimation(ShooterHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
	}
}

/*
* Funcion que desactiva aviso de mala conexion
*/
void AShooterPlayerController::StopHighPingWarning()
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	if (ShooterHUD &&									//Tener en cuenta que para evitar nullptr exception primero check del ShooterHUD, si existe,
		ShooterHUD->CharacterOverlay &&					//Comprobamos si existe la variable dentro de ShooterHUD y, si finalmente existe esa variable,
		ShooterHUD->CharacterOverlay->HighPingImage &&  //Comprobamos la variables dentro de CharacterOverlay. Alterar este orden puede provocar nullptr except.
		ShooterHUD->CharacterOverlay->HighPingAnimation)
	{
		ShooterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (ShooterHUD->CharacterOverlay->IsAnimationPlaying(ShooterHUD->CharacterOverlay->HighPingAnimation))
		{
			ShooterHUD->CharacterOverlay->StopAnimation(ShooterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}


/*
* Funcion que recoge del game mode el estado de la partida, esto para actualizar los tiempos de partida en cada jugador
*/
void AShooterPlayerController::ServerCheckMatchState_Implementation()
{
	AShooterGameMode* GameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));

	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		//Apunte: En lugar de hacer todas las variables replicadas, se usara un RTC ya que solo se enviaran las variables al principio por lo que no hay problema
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		//Si el matchstate = Waiting; se añade el overlay de waiting to start
		if (ShooterHUD && MatchState == MatchState::WaitingToStart)
		{
			ShooterHUD->AddAnnouncement();
		}
	}
}


void AShooterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;

	OnMatchStateSet(MatchState);

	//Si el matchstate = Waiting; se añade el overlay de waiting to start
	if (ShooterHUD && MatchState == MatchState::WaitingToStart)
	{
		ShooterHUD->AddAnnouncement();
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
	
	SingleTripTime = (0.5f * RoundTripTime);
	
	//Teniendo que el delay generado es por el trayecto Cliente->Servidor y Servidor->Cliente, si divides ese delay por la mitad y se lo sumas al tiempo
	//Que te ha dado el servidor, tendrás, aproximadamente, el tiempo que tiene el servidor al momento de leer esta linea de codigo
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;

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
		HandleMatchHasStarted();
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
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
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

/*
* Funcion que ejecuta la logica que conlleva que la fase de la partida haya cambiando a "InProgress"
* 1) Crear el HUD in game
* 2) Invisibilizar el HUD de espera
*/
void AShooterPlayerController::HandleMatchHasStarted()
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	if (ShooterHUD)
	{
		
		if(ShooterHUD->CharacterOverlay == nullptr) ShooterHUD->AddCharacterOverlay();

		//Se comprueba si announcement (tiempo para que empiece la partida) está generado, si lo está, se hace invisible debido a que ya empezó la partida
		//No se borra porque al final de la partida se volvera a usar el widget (para no estar creando y borrando)
		if (ShooterHUD->Announcement)
		{
			ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

/*
* Funcion que ejecuta la logica cuando se acaba una partida (mostrar ganador, puntuaciones, etc...)
*/
void AShooterPlayerController::HandleCooldown()
{
	ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;

	if (ShooterHUD)
	{
		ShooterHUD->CharacterOverlay->RemoveFromParent();

		//Se activa la visibilidad del HUD Announcement para mostrar la puntuacion
		if (ShooterHUD->Announcement && 
			ShooterHUD->Announcement->AnnouncementText &&
			ShooterHUD->Announcement->InfoText)
		{
			ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			FString AnnouncementText("New Match Starts In:");
			ShooterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			//Recogemos el valor del GameState, ya que aqui se guardan el ganador de la partida
			AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
			//Recogemos el valor del playerState del cliente, asi podemos comprobar si hemos ganado nosotros
			AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();

			if (ShooterGameState && ShooterPlayerState)
			{
				TArray<AShooterPlayerState*> TopPlayers = ShooterGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("No Winner");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == ShooterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win: \n");
					//recorremos los jugadores ganadores
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				ShooterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
	if (ShooterCharacter && ShooterCharacter->GetCombatComponent())
	{
		ShooterCharacter->bDisableGameplay = true;
		ShooterCharacter->GetCombatComponent()->FireButtonPressed(false); //Si esta disparando cuando acaba la partida se pone a false a la fuerza
	}

}

float AShooterPlayerController::GetPing()
{
	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
	if (PlayerState)
	{
		return PlayerState->GetPingInMilliseconds();
	}
	return -1.f;
}

/*
* Funcion llamada desde el GameMode, esta función llama a todos los clientes para que desde cada cliente, con la funcion ClientElimAnnouncement se ejecute la logica a nivel local de que se ha eliminado a un player
* Al hacerlo de esta manera podemos editar los mensajes a nivel local para hacerlos mas personalizados (Ej: tu eliminaste a X jugador)
*/
void AShooterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker,Victim);
}

/*
* Funcion que se llama a nivel cliente (UFUNCTION(Client)). Lleva la logica de imprimir por pantalla quien mato a quien a nivel local
*/
void AShooterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		ShooterHUD = ShooterHUD == nullptr ? GetHUD<AShooterHUD>() : ShooterHUD;
		if (ShooterHUD)
		{
			//Comprobamos posibilidades de matar a alguien o nos matan o nos suicidamos etc...
			if (Attacker == Self && Victim != Self)
			{
				ShooterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				ShooterHUD->AddElimAnnouncement("You", "yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}

			//Caso General:
			ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}