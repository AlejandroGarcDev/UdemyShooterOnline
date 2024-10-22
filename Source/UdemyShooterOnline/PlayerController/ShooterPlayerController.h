// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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

	virtual void OnPossess(APawn* InPawn) override;

protected:

	virtual void BeginPlay() override;

private:

	//Se utiliza UPROPERTY() porque al hacerlo se inicializa como nullptr, lo que permite que si no esta inicializado no tenga informacion random
	//Otra forma es inicializarlo como nullptr => class AShooterHUD* ShooterHUD; = nullptr 
	UPROPERTY()
	class AShooterHUD* ShooterHUD;
};
