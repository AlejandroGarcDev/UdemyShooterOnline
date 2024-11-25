// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY();

public:
	
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	float CrosshairSpread;
	FLinearColor CrosshairColor;
};


/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;

	//Guardamos el tipo de clase que es el widget CharacterOverlay
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	//Al tener la variable que define que clase es CharacterOverlayClass podemos crear una instancia y guardarla aqui
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	//Funcion que crea una instancia de la clase guardada en CharacterOverlayClass y la guarda en CharacterOverlay
	void AddCharacterOverlay();


	//Misma logica que CharacterOverlayClass
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<class UUserWidget> AnnouncementClass;

	//Misma logica que CharacterOverlay
	UPROPERTY()
	class UAnnouncement* Announcement;

	void AddAnnouncement();

	void AddElimAnnouncement(FString Attacker, FString Victim);

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	class APlayerController* OwningPlayer;

	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.0f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 3.5f;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;

public:

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
