// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "ShooterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	void SetHUDHealth(float Health, float MaxHealth);

	void SetHUDScore(float Score);

	void SetHUDDefeats(int32 Defeats);

	void SetHUDWeaponAmmo(int32 WeaponAmmo);

	void SetHUDCarriedAmmo(int32 CarriedAmmo);

	void SetHUDWeaponType(EWeaponType WeaponEquipped);

	void SetHUDMatchCountdown(float Time);

	virtual void OnPossess(APawn* InPawn) override;

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); //Synced with server world clock

	virtual void ReceivedPlayer() override; //We override this func to sync with server clock as soon as possible

	void OnMatchStateSet(FName State);

protected:

	virtual void BeginPlay() override;

	void SetHUDTime();

	void PollInit();

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

private:

	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	//Otra forma es inicializarlo como nullptr => class AShooterHUD* ShooterHUD; = nullptr 
	UPROPERTY()
	class AShooterHUD* ShooterHUD;

	//Temporal, esto irá en gamemode
	float MatchTime = 120.f;

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

	bool bInitializeCharacterOverlay = false;

	/*
	*	Estas variables se usan para guardar la informacion del HUD antes de que empiece la partida 
	*/

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};
