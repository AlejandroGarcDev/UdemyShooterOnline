// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UdemyShooterOnline/HUD/ShooterHUD.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "UdemyShooterOnline/ShooterTypes/CombatState.h"
#include "CombatComponent.generated.h"



class AMasterWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UDEMYSHOOTERONLINE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UCombatComponent();

	//Esto permite a la clase que pueda acceder a todas las funciones y variables aunque sean privadas o protegidas
	friend class AShooterCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AMasterWeapon* WeaponToEquip);

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

protected:

	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);

	void Fire();

	/*
	*	Con esta funcion llamamos desde el cliente al servidor para informar que hemos disparado, despues, el servidor hara un broadcast a los clientes.
	*	
	*	FVector_NetQuantize se usa para trasladar inf. a traves de la red
	* 
	*	FVector_NetQuantize tiene 0 decimales de precision
	*	Hasta 20 bits por component
	*	Valid Range -> 2^20 = +/- 1,048,576
	* 
	*/
	UFUNCTION(Server, Reliable)											//Reliable comprueba que se ha propagado el metodo al servidor
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);			//Tiene el prefijo Server para identificar que es una funcion dedicada al servidor

	//Funcion que se llama cuando el personaje muere y se quiera que muera disparando
	void DeathFire();

	void DeathFireTimer();

	//Una vez llamemos a ServerFire, el servidor llamara a esta funcion, que hara un broadcast a todos los clientes y al propio servidor tmb
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();

private:

	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	//Otra forma es inicializarlo como nullptr => class AShooterPlayerController* Controller; = nullptr 

	//Esto se inicializa en shooter character PostInitializeComponents
	UPROPERTY()
	class AShooterCharacter* Character;
	UPROPERTY()
	class AShooterPlayerController* Controller;
	UPROPERTY()
	class AShooterHUD* HUD;


	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AMasterWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	/*
	* HUD and Crosshairs
	*/

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	
	UPROPERTY(EditAnywhere, Category = Crosshair)
	float CrosshairInAirFactorMAX;

	float CrosshairAimFactor;
	
	UPROPERTY(EditAnywhere, Category = Crosshair)
	float CrosshairAimFactorMAX; //Factor que descompensa el crosshair como base para que al apuntar se vea bien

	float CrosshairShootingFactor; //Factor que descompensa el crosshair al disparar

	float CrosshairShootInterpSpeed; //Velocidad con la que se reajusta el crosshair al disparar

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/*
	* Aiming and FOV
	*/

	//FOV when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/**
	* Automatic Fire
	*/

	FTimerHandle FireTimer;
	
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	/*
	* Funcion que indica si puede disparar o no el character que posee este componente
	*/
	bool CanFire();
	
	/*
	* Municion que tiene el personaje del mismo tipo del arma (si tiene)
	*/
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	//Informacion: Por como funciona un TMap no es replicable en Unreal
	//Esto porque usa un tipo de algoritmo que puede dar resultados diferentes en distintas maquinas
	//Para nuestro proyecto, no importa, ya que solo queremos que sea replicable el CarriedAmmo
	//La municion del personaje en relacion al arma equipada
	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, uint32> CarriedAmmoMap;		

	//Variable que define las balas que tiene el personaje al principio
	UPROPERTY(EditAnywhere)
	int32 StartARAmmo = 30;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

public:	
		
};
