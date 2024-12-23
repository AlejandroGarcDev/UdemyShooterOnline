// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "UdemyShooterOnline/ShooterTypes/Team.h"
#include "MasterWeapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName="Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Secondary UMETA(DisplayName = "Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMAX") //Indica el numero de states
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")

};


UCLASS()
class UDEMYSHOOTERONLINE_API AMasterWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMasterWeapon();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;

	void SetHUDAmmo();

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const FVector& HitTarget); //Usamos "&" para evitar usar una copia, y usar la referencia de esa variable
												 //Usamos virtual para decirle al compilador que es una funcion que se sobrescribe en las clases hijo


	virtual void Dropped();

	void AddAmmo(int32 AmmoToAdd);

	FVector TraceEndWithScatter(const FVector& HitTarget);

	/*
	* Textures for the weapon crosshairs
	*/

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsBottom;


	/**
	*	Factor que dispersa el crosshair del personaje al disparar
	*	Tipicamente 0,5
	*
	*/
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairShootingSpread;

	/**
	*	Velocidad a la que se reajusta el crosshair del personaje al disparar
	*	Tipicamente 40
	* 
	*/
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairShootInterpSpeed;

	/**
	*	Velocidad a la que se reduce/contrae el crosshair del personaje al apuntar
	*	Valores de Referencia: Lento = 10 ;  MuyRapido = 40
	*
	*/
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairAimInterpSpeed;



	/*
	* Zoomed FOV While Aiming
	*/

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*
	* Delay entre disparos, depende del arma que se este usando
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.5f;

	/*
	* Variable que define si es automatica o no el arma
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;	

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;


	/**
	* Enable or Disable custom depth (used to make outlines)
	*/

	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();

	virtual void OnEquipped();

	virtual void OnEquippedSecondary();

	virtual void OnDropped();
	
	//Esta funcion es la que nos indicara cuando el jugador esta dentro del area del arma para recogerla
	//Los parametros de entrada vienen dados por el padre AActor
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	/*
	* Variables para la funcion "TraceEndWithScatter"
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UPROPERTY()
	class AShooterCharacter* ShooterOwnerCharacter;

	UPROPERTY()
	class AShooterPlayerController* ShooterOwnerController;

	UFUNCTION() //Importante que tenga UFUNCTION porque esta funcion se bindea con un delegate, si no, no se bindeara correctamente
	void OnPingTooHigh(bool bPingTooHigh);

private:

	UPROPERTY(VisibleAnywhere, Category="WeaponProperties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "WeaponProperties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(uint32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(uint32 AmmoToAdd);

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;
	
	//The number of unprocessed server requests for Ammo
	//Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	ETeam Team;
public:	

	void SetWeaponState(EWeaponState State);

	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetCrosshairShootingSpread() const { return CrosshairShootingSpread; }
	FORCEINLINE float GetCrosshairShootInterpSpeed() const { return CrosshairShootInterpSpeed; }

	bool IsEmpty();

	FORCEINLINE EWeaponType GetWeaponType()const { return WeaponType; }
	FORCEINLINE int32 GetAmmo()const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
