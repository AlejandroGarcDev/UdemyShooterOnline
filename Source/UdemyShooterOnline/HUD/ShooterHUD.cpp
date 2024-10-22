// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();

	AddCharacterOverlay();
}

void AShooterHUD::AddCharacterOverlay()
{	

	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();

	}
}


void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	//Obtenemos el tamaño de la pantalla
	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		//Guardamos el centro de la pantalla
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairCenter)
		{
			//El centro del puntero del arma no tiene dispersion/extension
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairLeft)
		{
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairRight)
		{
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairTop)
		{
			//En UV Texture Arriba es -Y
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairBottom)
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
	}

}


/*
* Funcion que pinta la textura en el centro de la pantalla
*/
void AShooterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	//Vector que recoge el punto (0,0) donde se debe poner la textura para que esté perfectamente centrada (0,0) (es el primer pixel de arriba izq.)
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.0f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.0f) + Spread.Y);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.0f,
		1.0f,
		CrosshairColor
	);
}
