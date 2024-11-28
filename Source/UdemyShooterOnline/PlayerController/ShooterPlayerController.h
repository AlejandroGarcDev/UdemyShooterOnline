// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "ShooterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class UInputAction;

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(uint32 RedScore);
	void SetHUDBlueTeamScore(uint32 BlueScore);

	void SetHUDWeaponAmmo(int32 WeaponAmmo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDWeaponType(EWeaponType WeaponEquipped);

	void SetHUDMatchCountdown(float Time);
	void SetHUDAnnouncementCountdown(float Time);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); //Synced with server world clock

	virtual void ReceivedPlayer() override; //We override this func to sync with server clock as soon as possible

	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	void HandleMatchHasStarted(bool bTeamsMatch = false);

	void HandleCooldown();

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	float GetPing();

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);
protected:

	virtual void BeginPlay() override;

	void SetHUDTime();

	void PollInit();

	virtual void SetupInputComponent() override;

	/**
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

	void HighPingWarning();

	void StopHighPingWarning();
	
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* QuitAction;

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	UFUNCTION()
	void OnRep_ShowTeamScores();

	FString GetInfoText(const TArray<class AShooterPlayerState*>& Players);
	FString GetTeamsInfoText(class AShooterGameState* ShooterGameState);
private:

	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	//Otra forma es inicializarlo como nullptr => class AShooterHUD* ShooterHUD; = nullptr 

	/**
	*	Return to main menu
	*/

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	UPROPERTY()
	class AShooterHUD* ShooterHUD;

	class AShooterGameMode* ShooterGameMode;

	//Variable que define el tiempo de una partida, esta variable es cambiado en la funcion ServerCheckMatchState
	float MatchTime = 0.f;

	float WarmupTime = 0.f;

	float CooldownTime = 0.f;

	float LevelStartingTime = 0.f;

	uint32 CountdownInt=0;

	/* Variable que nos indica estado de la partida(en progreso, iniciado, finalizado, abortado... ver GameMode.cpp MatchState)
	*  Teniendo en cuenta que nos 
	*/
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	/*
	*	Estas variables se usan para guardar la informacion del HUD antes de que empiece la partida 
	*/

	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;

	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;

	float HUDScore;
	bool bInitializeScore = false;

	int32 HUDDefeats;
	bool bInitializeDefeats = false;

	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;

	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	//Tiempo que lleva el jugador siendo avisado de que tiene mala conexion
	float HighPingRunningTime = 0.f; 

	//Tiempo del que se avisa al jugador que tiene mala conexion, esto para no estar todo el rato molestando con lo mismo
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;	

	float PingAnimationRunningTime = 0.f;

	//Cada cuanto tiempo se recuerda al jugador que tiene mala conexion
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

};
