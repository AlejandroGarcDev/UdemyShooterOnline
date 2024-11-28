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

	void SwapWeapon();

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();

	void FireButtonPressed(bool bPressed);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	UFUNCTION(BlueprintCallable)
	ECombatState GetCombatState();

protected:

	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void Fire();

	void FireProjectileWeapon();
	
	void FireHitScanWeapon();

	void FireShotgun();

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);

	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

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

	UFUNCTION(Server, Reliable)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime); 

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void PlayEquipWeaponSound(AMasterWeapon* WeaponToEquip);

	void HandleReload();

	void AttachActorToBack(AActor* ActorToAttach);

	void AttachFlagToBack(AActor* FlagToAttach);

	void AttachActorToRightHand(AActor* ActorToAttach);

	void UpdateCarriedAmmo();

	int32 AmountToReload();

	void EquipPrimaryWeapon(AMasterWeapon* WeaponToEquip);

	void EquipSecondaryWeapon(AMasterWeapon* WeaponToEquip);

	void EquipFlag(class AFlag* FlagToEquip);

	bool bLocallyReloading = false; //Variable de conciciliacion con el servidor a la hora de recargar
									//Si hay bastante lag, el servidor al propagar que esta recargando al cliente que controla el character, puede ser
									//que el character ya haya terminado de recargar, por lo que localmente se comprueba esta variable

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

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AMasterWeapon* SecondaryWeapon;

	UPROPERTY()
	AFlag* FlagEquipped;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;

	//Esta variable solo se actualiza localmente y tiene la funcion de verificar si la informacion que le llega al cliente desde el servidor es correcta
	//Por motivos de lag cuando el servidor le dice al cliente que esta apuntando puede ser que haya dejado de apuntar, entonces se comprueba esta variable para comprobar este tipo de casos
	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed=false;

	bool bFireOnCooldown=false;

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
	
	//Variable para gestionar el disparo automatico y manual
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
	
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	//Variable que define las balas que tiene el personaje al principio AR=AssaultRifle
	UPROPERTY(EditAnywhere)
	int32 StartARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	uint32 StartGrenadeLauncherAmmo = 0;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

	UPROPERTY(Replicated)
	bool bHoldingTheFlag = false;

public:	
		
	bool ShouldSwapWeapons();
};
