// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UdemyShooterOnline/Weapon/MasterWeapon.h"
#include "UdemyShooterOnline/ShooterComponents/CombatComponent.h"
#include "UdemyShooterOnline/ShooterComponents/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ShooterAnimInstance.h"
#include "UdemyShooterOnline/UdemyShooterOnline.h"
#include "UdemyShooterOnline/PlayerController/ShooterPlayerController.h"
#include "UdemyShooterOnline/GameMode/ShooterGameMode.h"
#include "TimerManager.h"
#include "Misc/App.h"
#include "UdemyShooterOnline/PlayerState/ShooterPlayerState.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "UdemyShooterOnline/ShooterComponents/LagCompensationComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UdemyShooterOnline/GameState/ShooterGameState.h"
#include "UdemyShooterOnline/Weapon/Flag.h"
#include "UdemyShooterOnline/PlayerStart/TeamPlayerStart.h"


// Sets default values
AShooterCharacter::AShooterCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComponent->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));


	//If true, pawn is capable of crouching
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletonMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);				//Las camaras no detectan la malla del personaje
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);				//Todo lo que sea visibility sera bloqueado por la malla del personaje

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //Inicializamos valor de giro como no girar

	NetUpdateFrequency = 66.0f;		//Cuantas veces por segundo se actualiza el personaje en el servidor
	MinNetUpdateFrequency = 33.0f;  //Cuantas veces como minimo se actualiza el personaje si hay restriccion de ancho de banda

	/**
	*	Hit boxes for server-side rewind
	*/

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	thigh_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

}

void AShooterCharacter::Reset()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}

	Super::Reset();
}

void AShooterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;
	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void AShooterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void AShooterCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr) return;

	switch (Team)
	{
	case ETeam::ET_NoTeam:
		if (NoTeam_ColorInstance_01 == nullptr || NoTeam_ColorInstance_02 == nullptr) return;
		GetMesh()->SetMaterial(0, NoTeam_ColorInstance_01);
		GetMesh()->SetMaterial(1, NoTeam_ColorInstance_02);
		break;
	case ETeam::ET_BlueTeam:
		if (BlueTeam_ColorInstance_01 == nullptr || BlueTeam_ColorInstance_02 == nullptr) return;
		GetMesh()->SetMaterial(0, BlueTeam_ColorInstance_01);
		GetMesh()->SetMaterial(1, BlueTeam_ColorInstance_02);
		break;
	case ETeam::ET_RedTeam:
		if (RedTeam_ColorInstance_01 == nullptr || RedTeam_ColorInstance_02 == nullptr) return;
		GetMesh()->SetMaterial(0, RedTeam_ColorInstance_01);
		GetMesh()->SetMaterial(1, RedTeam_ColorInstance_02);
		break;	
	}
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(CharacterMappingContext, 0);
			bInputsSet = true;
		}
	}

	UpdateHUDHealth();
	UpdateHUDShield();

	if (HasAuthority())	//Solo entra el servidor en este if
	{
		OnTakeAnyDamage.AddDynamic(this, &AShooterCharacter::ReceiveDamage); //Se bindea los eventos de recibir daño de esta clase a la funcion ReceiveDamg
	}

}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Codigo para comprobar que tiene los controles seteados (esto se realiza en el begin play)

	if (HasAuthority() && !bInputsSet && Controller)
	{
		ShooterPlayerController = !ShooterPlayerController ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
		if (ShooterPlayerController)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(ShooterPlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(CharacterMappingContext, 0);
				bInputsSet = true; //successful
			}
		}
	}



	/*
	* Simulated Proxy es un actor que no esta siendo controlado de manera local
	* Si comprobamos ENetRole veremos que lo que esta por encima de SimulatedProxy es:
	* 1) Autonomous Proxy: Jugador activo de manera local
	* 2) Authority: servidor
	* 3) MAX: servidor
	*/
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}

	if (IsLocallyControlled())
	{
		CameraBoomCrouch(DeltaTime);
		HideCameraIfCharacterClose();
	}

	PollInit();
}

/*
* Funcion que se utiliza para replicar variables
*/
void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Este macro se utiliza para replicar variable desde el servidor a los cliente, al poner el "_CONDITION"
	//Indicamos una condicion para el replicamiento, en este caso como esta linea es para ver
	//El pick up del arma, queremos que se replique solo en el que genera este comando, es decir,
	//El que solapa con el area, eso lo conseguimos con OwnerOnly, ya que Owner es el dueño del solapamiento
	//Nota: Esta funcion es llamada cuando se utiliza el setter en el .h ya que al cambiar la variable overlapping esta funcion se ejecuta
	//ya que es una funcion inherente debido al replicamiento
	DOREPLIFETIME_CONDITION(AShooterCharacter, OverlappingWeapon, COND_OwnerOnly);

	DOREPLIFETIME(AShooterCharacter, Health);
	DOREPLIFETIME(AShooterCharacter, Shield);

	DOREPLIFETIME(AShooterCharacter, bDisableGameplay);
}

void AShooterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

/*
* Funcion que se llama desde el game mode, por lo tanto solo se llama en el servidor (Gamemode solo existe en el servidor)
* En esta funcion estan las operaciones que necesita el servidor cuando se elimina un jugador y un multicast de la animacion de muerte para todos los clientes
*/
void AShooterCharacter::Elim(bool bPlayerLeftGame)
{
	MulticastElim(bPlayerLeftGame);
}

void AShooterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	//Seteamos la variable Ammo del HUD del jugador en 0 para resetearlo, ya que al morir pierde el arma
	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDWeaponAmmo(0);
	}

	bElimmed = true; //Variable que se utiliza para que no se ejecuten dos animaciones (ser golpeado y morir)

	GetMesh()->SetCollisionObjectType(ECC_Pawn);



	PlayElimMontage();

	//Deshabilitamos el movimiento del personaje
	GetCharacterMovement()->DisableMovement();				//Deshabilitamos WSAD
	GetCharacterMovement()->StopMovementImmediately();		//Deshabilitamos movimiento mediante la rotacion del personaje

	//Si estaba apuntando se muere pegando tiros al aire
	if (IsWeaponEquipped() && IsAiming())
	{
		Combat->DeathFire();
	}

	//Deshabilitamos los input actions
	bDisableGameplay = true;

	//Deshabilitamos colisiones **IMPORTANTE: no se deshabilitan las de la malla (GetMesh()) porque se simulan fisicas de caida**
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Si muere apuntando con el franco se esconde el HUD del franco
	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	GetWorldTimerManager().SetTimer(
		ElimTimer,									//Parametro1: FTimerHandle
		this,										//Parametro2: Objeto que activa el timer (Esto se llama en el servidor)
		&AShooterCharacter::ElimTimerFinished,		//Parametro3: Funcion que se activa al acabar el timer
		ElimDelay,									//Parametro4: Tiempo
		false,										//Parametro5: Loop
		-1.0f										//Parametro6: Delay en el primer Loop
	);

}

/*
* Funcion que se llama desde Elim(), esta funcion se llama cuando ya ha pasado el tiempo de la muerte y se requiere que reaparezca el character
*/
void AShooterCharacter::ElimTimerFinished()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
		if (Combat->FlagEquipped)
		{
			Combat->FlagEquipped->SetWeaponState(EWeaponState::EWS_Dropped);
		}
	}

	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	if (ShooterGameMode && !bLeftGame)
	{
		ShooterGameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void AShooterCharacter::ServerLeaveGame_Implementation()
{
	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	ShooterPlayerState = ShooterPlayerState == nullptr ? Cast<AShooterPlayerState>(GetPlayerState()) : ShooterPlayerState;
	if (ShooterGameMode && ShooterPlayerState)
	{
		ShooterGameMode->PlayerLeftGame(ShooterPlayerState);
	}
}

/*
* Funcion que comprueba si un arma es inicial o es cogida durante el gameplay, segun el caso se destruye o no el arma
* Esta funcion se llama al finalizar el tiempo de muerte, es decir, cuando desaparece el character tras morir (ElimTimerFinished)
*/
void AShooterCharacter::DropOrDestroyWeapon(AMasterWeapon* Weapon)
{
	if (Weapon == nullptr) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

/*
* Funcion que se ejecuta en el instante despues de asignar equipo al character (PollInit()), esta funcion asigna un punto de reaparicion en funcion del equipo asignado
*/
void AShooterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && ShooterPlayerState && ShooterPlayerState->GetTeam() != ETeam::ET_NoTeam) //Si somos el server y este character tiene un equipo asignado...
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts); //(Actor del mundo, actores a localizar, donde guardar los actores)
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == ShooterPlayerState->GetTeam()) //Si el TeamStart es del equipo del character, se guarda como posible spawn
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}

		if (TeamPlayerStarts.Num()>0) //Si existen Spawns del mismo equipo...
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)]; //Escoge uno aleatorio
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				ChosenPlayerStart->GetActorRotation()
			);
		}
	}

}


// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Ahora vamos a Bindear los Inputs Actions
	//de esta manera compruebo con CastChecked si PlayerInputComponent tiene el componente UEnhancedInputComponent, si falla, el metodo
	//castChecked crashea el juego
	if (UEnhancedInputComponent* EnhacedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//En esta linea de codigo utilizo el puntero MoveAction, inicializado en el editor ya que tiene "EditAnywhere", dentro del propio InputAction indico
		//Que tecla utilizo para activarlo (ver dentro de Unreal), en esta funcion se linkean el trigger del InputAction con la funcion Move,
		EnhacedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		//misma logica para esta linea
		EnhacedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		//misma logica para esta linea
		EnhacedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Jump);

		EnhacedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ThisClass::EquipButtonPressed);

		EnhacedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::CrouchButtonPressed);

		EnhacedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::AimButtonPressed);

		EnhacedInputComponent->BindAction(FireAction, ETriggerEvent::Ongoing, this, &ThisClass::FireButtonPressed);

		EnhacedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);

		EnhacedInputComponent->BindAction(ReloadAction, ETriggerEvent::Completed, this, &ThisClass::ReloadButtonPressed);
	}

	//Ponemos opcion de agacharse como true por defecto para que el personaje pueda agacharse
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

/*
* Funcion heredada de Character, se ejecuta cuando sus componentes estan inicializadas (en BeginPlay puede que no), se utiliza para registrar valores iniciales,
* Como los valores de velocidad iniciales del personaje
*/
void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat -> Character = this;
	}
	if (BuffComponent)
	{
		BuffComponent->Character = this;

		BuffComponent->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed, 
			GetCharacterMovement()->MaxWalkSpeedCrouched);

		BuffComponent->SetInitialJumpVelocity(
			GetCharacterMovement()->JumpZVelocity
		);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;

		if (Controller)
		{
			LagCompensation->Controller = Cast<AShooterPlayerController>(Controller);
		}
	}
}

void AShooterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon==nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);

		//En funcion de si estoy disparando o no recojo una parte del montage u otra (ver en Unreal Engine)
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");

		//Se ejecuta la animacion
		AnimInstance->Montage_JumpToSection(SectionName);
	}

}

void AShooterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);

		//En funcion del arma que tengo equipada utilizo una parte del montage u otra
		FName SectionName;
		
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("Rifle");
			break;

		}

		//Se ejecuta la animacion
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void AShooterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		
		//En funcion de por donde me dispara se ejecuta una animacion u otra del montage
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void AShooterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	if (bElimmed || ShooterGameMode == nullptr) return;
	Damage = ShooterGameMode->CalculateDamage(InstigatorController, Controller, Damage);
	/*
	* Esto se ejecuta en el servidor (Ver BeginPlay)
	* Sin embargo, como Health es una variable replicada:
	* Paso 1: UPROPERTY(ReplicatedUsing)
	* Paso 2: Incluir esa variable en la funcion GetLifetimeReplicatedProps
	* Se actualizará en todos los clientes igualmente (cada cliente con su widget)
	*/

	/*
	* Este metodo es mejor que hacer un multicast RPC (NetMulticast Unreal)
	*/

	float DamageRemainder = Damage;
	float LastShield = Shield;

	Shield = FMath::Clamp(Shield - DamageRemainder, 0.f, MaxShield);

	DamageRemainder = DamageRemainder + Shield - LastShield; //El daño sobrante es el daño total - la diferencia de escudo quitado
	
	if (DamageRemainder > 0.f)
	{
		Health = FMath::Clamp(Health - DamageRemainder, 0.f, MaxHealth);
	}

	PlayHitReactMontage();

	UpdateHUDShield();
	UpdateHUDHealth();


	if (Health == 0.0f)
	{
		if (ShooterGameMode)
		{
			ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
			AShooterPlayerController* AttackerController = Cast<AShooterPlayerController>(InstigatorController);
			ShooterGameMode->PlayerEliminited(this, ShooterPlayerController, AttackerController);
		}
	}
}

void AShooterCharacter::CameraBoomCrouch(float DeltaTime)
{
	//CameraBoomSocketOffset.Z Inicial en Z = 30.f;		Al agacharse se le resta 50 => CameraBoomSocketOffset.Z = 30.f;

	if (bIsCrouched)
	{
		CameraBoom->SocketOffset.Z = FMath::FInterpTo(CameraBoom->SocketOffset.Z, 30.f, DeltaTime, 10.f);
	}
	else
	{
		CameraBoom->SocketOffset.Z = FMath::FInterpTo(CameraBoom->SocketOffset.Z, 80.f, DeltaTime, 10.f);
	}
	
	
}

/*
* Funcion que inicializa las variable de PlayerState
* En los primeros momentos del character PlayerState todavia no está asignado por lo que esta funcion se ejecuta en Tick para que pregunte todo el tiempo
*/
void AShooterCharacter::PollInit()
{
	if (ShooterPlayerState == nullptr)
	{
		ShooterPlayerState = Cast<AShooterPlayerState>(GetPlayerState());
		//Solo entrara aqui una vez, el momento en el que se puede inicializar la variable ShooterPlayerState
		if (ShooterPlayerState)
		{
			ShooterPlayerState->AddToScore(0.f); //Una vez tenemos PlayerState actualizamos la puntuacion en pantalla
			ShooterPlayerState->AddToDefeats(0);
			SetTeamColor(ShooterPlayerState->GetTeam()); //Una vez tenemos PlayerState podemos saber en que equipo estamos y por tanto asignarnos un color
			SetSpawnPoint();

			//Comprobamos si necesitamos la corona (en caso de ser los primeros de la partida)
			AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
			if (ShooterGameState && ShooterGameState->TopScoringPlayers.Contains(ShooterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}


void AShooterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();
	if (GetController())
	{
		AddControllerYawInput(LookAxisValue.X * SensibilidadX);
		AddControllerPitchInput(LookAxisValue.Y * SensibilidadY);
	}
}


void AShooterCharacter::Move(const FInputActionValue& Value)
{

	if (bDisableGameplay) return;

	const FVector2D DirectionValue = Value.Get<FVector2D>();
	
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, DirectionValue.Y);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, DirectionValue.X);
}

void AShooterCharacter::Jump()
{
	if (bDisableGameplay) return;
	Super::Jump();
}

void AShooterCharacter::EquipButtonPressed()
{
	if(bDisableGameplay) return;
	if (Combat)
	{		
		//A nivel cosmetico local, se puede ejecutar sin llamar al servidor 
		if (IsLocallyControlled())
		{
			//Donde unico se activa algo a nivel local al recoger un arma, es cuando no hay secundaria, por lo que es lo unico que comprobamos.
			//(Si hay arma secundaria, se cambia el arma del suelo con la principal y no se activa el outline blanco)
			if (OverlappingWeapon != nullptr && OverlappingWeapon->GetWeaponType() != EWeaponType::EWT_Flag && Combat->SecondaryWeapon == nullptr)
			{
				OverlappingWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
				OverlappingWeapon->GetWeaponMesh()->MarkRenderStateDirty();
				OverlappingWeapon->EnableCustomDepth(true);
			}
			else if (OverlappingWeapon == nullptr && !HasAuthority() && Combat->ShouldSwapWeapons() && Combat->CombatState == ECombatState::ECS_Unoccupied)
			{
				//Este codigo se ejecuta en ServerEquipButtonPressed, por lo que para que no se ejecute 2 veces se hace la comprobacion de !HasAuthority
				PlaySwapMontage();
				Combat->CombatState = ECombatState::ECS_SwappingWeapon;
				bFinishedSwapping = false;

				/*
				* La logica de outline al swapear armas está dentro del montage (Combat->FinishSwapAttach)
				*/
			}
		}
		
		//A nivel de seguridad, solo el servidor puede asignar armar a los personajes
		ServerEquipButtonPressed();
	}
}

void AShooterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapon();
		}
	}
}

//Esta funcion se activara cuando haya un cambio en la variable "OverlappingWeapon" de acuerdo a la UProperty puesta,
//Este funcion simplemente activa o no el widget de pick up, esta funcion se activara unicamente en el cliente que este solapando
//con el area del arma debido a la funcion GetLifetimeReplicateProps utilizada en este .cpp
void AShooterCharacter::OnRep_OverlappingWeapon(AMasterWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}
void AShooterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;

	//Tipica logica de si esta sentado se levanta y si no se agacha
	//bIsCrouched es una variable heredada de character la cual tiene replication y tiene ya su propia logica con metodos como crouch y uncrouch
	if (bIsCrouched)
	{
		UnCrouch();
		
		//Se llama a la funcion SetAiming(bAiming) ya que dentro de ese codigo se actualiza la velocidad del personaje
		Combat->SetAiming(Combat->bAiming);

	}
	else
	{
		//Si vamos a la funcion "Crouch" vemos que hay una variable definida por Unreal bIsCrouched que tiene replicacion y se pondra a true
		//Por lo que no tenemos que llamar al servidor
		//Esto se utilizará para la animacion del personaje
		Crouch();

	}	
}

/*
* Funcion de recargar
*/
void AShooterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->Reload();
	}
}

void AShooterCharacter::AimButtonPressed()
{
	//bDisableGameplay=true cuando acaba la partida (por ejemplo)
	if (bDisableGameplay)
	{
		StopAiming();
		return;
	}

	if (Combat)
	{
		if (Combat->bAiming == false)
		{
			Combat->SetAiming(true);
			SensibilidadX = 0.4f;
			SensibilidadY = 0.4f;
		}
		else
		{
			StopAiming();
			SensibilidadX = 1.f;
			SensibilidadY = 1.f;
		}
	}
}

void AShooterCharacter::StopAiming()
{
	Combat->SetAiming(false);
}


float AShooterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}


/*
* La logica de esta funcion es:
* 
* 1) Activar/Desactivar YawControllRotation para que el personaje no rote automaticamente una vez movemos el raton en el eje Z.
* Se desactivará cuando el personaje esté quieto, para que pueda mirar para los lados solo cuando este quieto y, se activara, 
* cuando el personaje se esté moviendo
* 
* 2) Actualizar la diferencia entre la rotacion del personaje (YAW) y hacia donde apunta el jugador cuando el personaje esté quieto para
* poder decirle a la animacion la diferencia de grados
* 
* 3) Actualizar PITCH siempre
* 
*/
void AShooterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return; //Si no tenemos arma no hacemos nada ya que el AimOffset es cuando tenemos arma
	
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	float Speed = CalculateSpeed();

	if (Speed == 0.0f && !bIsInAir) //Cuando el personaje esta quieto y no esta en el aire
	{
		bRotateRootBone = true; // variable que se usa para actualizar la rotacion de los players ajenos al cliente

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartingAimRotation, CurrentAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir) // Cuando el personaje corre o salta
	{
		bRotateRootBone = true; // variable que se usa para actualizar la rotacion de los players ajenos al cliente

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);//Actualizamos todo el rato el valor de StartingAimRotation
																			//Para que cuando pare el PJ tenga el ultimo valor del angulo en Z
																			//Este valor se compara arriba para ver la diferencia de angulo
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning; //Si el personaje esta girando se establece como no girar, ya que está en el aire o moviendose
	}

	CalculateAO_Pitch();
}

void AShooterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	/*
	* Por como funciona los envios de angulos en Unreal, para enviar la info mas rapido, convierte de float a int16:
	*
	* Funcion GetCharacterMovement.cpp -> CompressAxisToShort -> return FMath::RoundToInt(Angle * 65536.f / 360.f ) & 0xFFFF;
	*
	* El problema con esto es que los valores negativos pasan a positivos --> -1º = 359º
	* Y al nosotros mirar hacia abajo localmente el servidor lo que recibe es que estamos mirando hacia arriba, por lo que tenemos que corregirlo
	*/
	if (AO_Pitch > 90.0f && !IsLocallyControlled())
	{
		//map pitch from [270,360) to [-90, 0)

		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);

		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);

	}
}

void AShooterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;

	if (CalculateSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	bRotateRootBone = false;
	CalculateAO_Pitch();
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw=UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if(ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void AShooterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AShooterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

/*
* Esta funcion sirve para saber cuando el personaje esta quieto y mirando a izq, o der. a mas de 90º, por lo que el personaje debera girar
* Esto se hará en AnimInstance.cpp
*/
void AShooterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 5.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.0f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); //Actualizamos todo el rato el valor de StartingAimRotation
																				//Para que cuando pare el PJ tenga el ultimo valor del angulo en Z
																				//Este valor se compara arriba para ver la diferencia de angulo
		}
	}
}


/*
* Funcion que invisibiliza el character si esta muy cerca de la camara (Ej: Pegado a una pared)
*/
void AShooterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	bool MeshVisibility = true;
	//En funcion de si es menor que un limite ponemos la variable de invisibilidad a true o false
	MeshVisibility = ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraTreshold) ? false : true;
	GetMesh()->SetVisibility(MeshVisibility);
	if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
		Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = !MeshVisibility;
	}
	if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh()) {
		Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = !MeshVisibility;
	}

	/*
	*					FORMA MAS LEGIBLE (MENOS COMPACTA)
	*		
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		//Se hace invisible el mesh del PlayerCharacter
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			//Se hace invisible el mesh del arma
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		//Se hace visible el mesh del PlayerCharacter
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			//Se hace visible el mesh del arma
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
	*/
}

/*

						Funcion que se llamaba en Projectile.cpp
						Borrada porque no es recomendable hacer un multicast

void AShooterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}


*/

/*
* Esta funcion se llama cuando se actualiza la variable Health (ver Shootercharacter.h)
*/
void AShooterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	if (!bElimmed && Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void AShooterCharacter::OnRep_Shield(float LastShield)
{

	UpdateHUDShield();

	if (!bElimmed && Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

/*
* Esta funcion llama al controlador y comprueba si es AShooterPlayerController, si lo es, utiliza la funcion actualizar vida del HUD
*/
void AShooterCharacter::UpdateHUDHealth()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

/*
* Esta funcion llama al controlador y comprueba si es AShooterPlayerController, si lo es, utiliza la funcion actualizar escudo del HUD
*/
void AShooterCharacter::UpdateHUDShield()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;

	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void AShooterCharacter::UpdateHUDAmmo()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;

	if (ShooterPlayerController && Combat && Combat->EquippedWeapon)
	{
		ShooterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		ShooterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

/*
* Funcion que equipa el arma por defecto al entrar en la partida (se identifica partida porque el game mode es unico)
*/
void AShooterCharacter::SpawnDefaultWeapon()
{
	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	UWorld* World = GetWorld();
	if (ShooterGameMode  && World && !bElimmed && DefaultWeaponClass)
	{
		AMasterWeapon* StartingWeapon = Cast<AMasterWeapon>(World->SpawnActor(DefaultWeaponClass));
		StartingWeapon->bDestroyWeapon = true; //Variable que sirve para identificar las armas iniciales, estas armas desaparecen si el jugador muere con ellas
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}

}

void AShooterCharacter::SetOverlappingWeapon(AMasterWeapon* Weapon)
{

	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	// Este if hace un chekeo en caso de que sea el servidor (que tambien tiene un personaje jugando) el que solapa con el arma, ya que la funcion
	// GetLifetimeReplicateProps solo funciona con clientes

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool AShooterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AShooterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AMasterWeapon* AShooterCharacter::GetEquippedWeapon()
{
	if(Combat==nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector AShooterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState AShooterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}

bool AShooterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}

bool AShooterCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr) return false;
	return Combat->bHoldingTheFlag;
}

ETeam AShooterCharacter::GetTeam()
{
	ShooterPlayerState = ShooterPlayerState == nullptr ? Cast<AShooterPlayerState>(GetPlayerState()) : ShooterPlayerState;
	if(ShooterPlayerState == nullptr) return ETeam::ET_NoTeam;
	return ShooterPlayerState->GetTeam();
}
