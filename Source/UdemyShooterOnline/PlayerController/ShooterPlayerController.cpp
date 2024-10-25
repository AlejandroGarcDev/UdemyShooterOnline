// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"
#include "UdemyShooterOnline/HUD/ShooterHUD.h"
#include "UdemyShooterOnline/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UdemyShooterOnline/Character/ShooterCharacter.h"
#include "UdemyShooterOnline/Weapon/WeaponTypes.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShooterHUD=GetHUD<AShooterHUD>();
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
		FString SWeaponEquipped;
		switch (WeaponEquipped)
		{
		case EWeaponType::EWT_AssaultRifle:
			SWeaponEquipped = "Assault Rifle";
			break;
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *SWeaponEquipped);

		ShooterHUD->CharacterOverlay->WeaponEquipped->SetText(FText::FromString(SWeaponEquipped));
	}


}



