// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "ElimAnnouncement.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();
}

/*
* Funcion que genera una instancia de la clase guardada en CharacterOverlayClass, la guarda en CharacterOverlay y se imprime por pantalla
*/
void AShooterHUD::AddCharacterOverlay()
{	

	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();

	}
}

/*
* Funcion que genera una instancia de la clase guardada en AnnouncementClass, la guarda en Announcement y se imprime por pantalla
*/
void AShooterHUD::AddAnnouncement()
{

	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && AnnouncementClass && Announcement == nullptr)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();

	}
}

/*
* Funcion que añade mensaje de eliminacion en el HUD del character,
* Además, también crea un timer para su desaparición 
*/
void AShooterHUD::AddElimAnnouncement(FString Attacker, FString Victim)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	
	if (OwningPlayer && ElimAnnouncementClass)
	{
		UElimAnnouncement* ElimAnnouncementWidget = CreateWidget<UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass);
		if (ElimAnnouncementWidget)
		{
			ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			ElimAnnouncementWidget->AddToViewport();

			//Al añadir un mensaje, movemos el resto de mensajes existentes para arriba
			for (UElimAnnouncement* Msg : ElimMessages)
			{
				if (Msg && Msg->AnnouncementBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Cast<UWidget>(Msg->AnnouncementBox));
					if (CanvasSlot)
					{
						FVector2D Position = CanvasSlot->GetPosition();
						FVector2D NewPosition(
							Position.X,
							Position.Y - CanvasSlot->GetSize().Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			ElimMessages.Add(ElimAnnouncementWidget);

			//Para que el mensaje creado se elimine al pasar X segundos, creamos un timer.
			//Pero esta vez la funcion que se ejecuta cuando acaba el timer tiene un parametro de entrada,
			//para llamar a una funcion con parametro de entrada creamos el delegate y en lugar de llamar a la funcion en el timer, llamamos al delegate
			//este delegate llamara a la funcion con el parametro de entrada indicado
			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget); //FName->Funcion ElimAnnouncementWidget->Parametro
			GetWorldTimerManager().SetTimer(
				ElimMsgTimer,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false
			);
		}
	}
}


void AShooterHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
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
