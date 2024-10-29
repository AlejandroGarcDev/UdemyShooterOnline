// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "UdemyShooterOnline/ShooterTypes/TurningInPlace.h"
#include "UdemyShooterOnline/Interfaces/InteractWithCrosshairsInterface.h"
#include "UdemyShooterOnline/ShooterTypes/CombatState.h"
#include "ShooterCharacter.generated.h"



class UInputMappingContext;
class UInputAction;

UCLASS()
class UDEMYSHOOTERONLINE_API AShooterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//En esta funcion se registran las variables para ser replicadas (en nuestro caso el arma)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Con esta funcion inicializamos los componentes del personaje,
	//Como necesita un componente de combate se hace override de esta funcion
	//Y se inicializa el componente en ella
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);

	void PlayReloadMontage();

	void PlayElimMontage();


	/*
			Funcion que se llamaba en Projectile.cpp
			Borrada porque no es recomendable hacer un multicast, en su lugar se ha

	* Funcion para propagar la animacion de golpeado de un Player Character
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();


	*/

	/*
	* Funcion inherente de Unreal, cuando un actor replicado cambia su posicion o velocidad se llama a un notify que ejecuta esta funcion
	* Se utiliza para replicar la rotacion del personaje, para que se vea fluido
	*/
	virtual void OnRep_ReplicatedMovement() override;

	void Elim();

	//Multicast de animacionde  morir de un personaje
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	virtual void Reset() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateHUDHealth();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* CharacterMappingContext; // InputMapContext que contiene IA_Move

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction; //Input Action para mover la camara

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction; //Input Action para moverse en X-Y

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction; //Input Action para moverse en X-Y

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ReloadAction;

	/**
	*	Animation montages
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;


	void Look(const FInputActionValue& Value);

	void Move(const FInputActionValue& Value);

	virtual void Jump() override;

	void EquipButtonPressed();

	void CrouchButtonPressed();

	void ReloadButtonPressed();

	void AimButtonPressed();

	void StopAiming();

	void AimOffset(float DeltaTime);

	void CalculateAO_Pitch();

	void SimProxiesTurn(); //Funcion que se llamara en simulated proxies (clientes), para actualizar los otros jugadores

	void FireButtonPressed();

	void FireButtonReleased();

	void PlayHitReactMontage();

	UFUNCTION() //Necesita ser UFUNCTION para que se llame al hacer daño (se bindea en begin play)
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController,AActor* DamageCauser);

	void CameraBoomCrouch(float DeltaTime);

	/*
	* Funcion que se ejecuta en Tick
	* 
	* Explicacion: Lo mas normal es que iniciemos el HUD del character en el BeginPlay, pero el PlayerState, que es quien tiene la puntuacion de la partida,
	* es posible que todavia no existe en el character, por lo que tenemos pedir en el Tick la informacion para ver si ya existe.
	* 
	* Checkeo de variables necesarias e inicializacion del HUD
	* 
	*/
	void PollInit();

private:

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true)) //Al ser una variable privada tenemos que ponerle el meta para acceder a ella en blueprints
	class UWidgetComponent* OverheadWidget; //Widget para mostrar el nombre encima del personaje

	//Esta variable es para guardar el arma que este cerca del jugador
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AMasterWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AMasterWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true)) //Al ser una variable privada tenemos que ponerle el meta para acceder a ella en blueprints
	class UCombatComponent* Combat;

	//Reliable es para confimar que se ha executado (protocolo de confirmacion para manejar funciones por internet). En funciones como el tick no es recomendado
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw; //Esta variable servira para girar el personaje cuando giremos estando quieto
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	void HideCameraIfCharacterClose();
	
	/*
	* Variable que define la distancia entre el PJ y la camara antes de hacer invisible al PJ
	*/
	UPROPERTY(EditAnywhere)
	float CameraTreshold = 200.f;

	bool bRotateRootBone;

	float TurnThreshold = 2.f; //Variable para ejecutar la rotacion de personajes en simulatedproxys, ya que en estos no se actualiza el AO_Yaw

	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed();

	/**
	* Player health
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class AShooterPlayerController* ShooterPlayerController;

	bool bElimmed = false;

	//Timer para cuando muere un player
	FTimerHandle ElimTimer;

	//EditDefaultsOnly te permite editarlo en Unreal pero solo el valor por defecto, asi no hay instancias con diferente tiempo de respawn
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.0f;

	void ElimTimerFinished();

	UPROPERTY()
	class AShooterPlayerState* ShooterPlayerState;

	UPROPERTY()
	bool bInputsSet = false;

public:

	// Setter llamado en MasterWeapon.cpp cuando haya solapamiento del AreaSphere y Character
	void SetOverlappingWeapon(AMasterWeapon* Weapon);

	bool IsWeaponEquipped();

	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	AMasterWeapon* GetEquippedWeapon();
	
	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	ECombatState GetCombatState() const;
};

