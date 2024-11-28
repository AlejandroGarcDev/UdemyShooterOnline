// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()
	
	UPROPERTY()
	TMap<AShooterCharacter*, uint32> HeadShot;
	
	UPROPERTY()
	TMap<AShooterCharacter*, uint32> BodyShot;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	AShooterCharacter* Character;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UDEMYSHOOTERONLINE_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	ULagCompensationComponent();

	friend class AShooterCharacter; //Así ShooterCharacter puede acceder a variables privadas de esta clase

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ShowFramePackage(const FFramePackage& Package,const FColor& Color);

	/*
	* Mediante esta funcion se comprueba en el servidor si el hit de un jugador cliente es valido, esto para que los jugadores con lag puedan tener mejor exp.
	*/
	FServerSideRewindResult ServerSideRewind(
		class AShooterCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime);

	/**
	*	Projectile
	*/
	FServerSideRewindResult ProjectileServerSideRewind(
		class AShooterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<AShooterCharacter*>& HitCharacters,
		const FVector_NetQuantize TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	/*
	* Funcion que se ejecuta en el servidor para verificar un hit en un character
	*/
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		AShooterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		AShooterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<AShooterCharacter*>& HitCharacters,
		const FVector_NetQuantize TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);
	
protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	void CacheBoxPosition(AShooterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(AShooterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	void SaveFramePackage();

	FFramePackage GetFrameToCheck(AShooterCharacter* HitCharacter, float HitTime);

	/**
	*	HitScan
	*/
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		AShooterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation);

	/**
	*	Projectile
	*/
	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		AShooterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);


	/**
	*Shotgun
	*/
	FShotgunServerSideRewindResult ShotgunCofirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);

private:

	UPROPERTY()
	AShooterCharacter* Character;

	UPROPERTY()
	class AShooterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;

public:	
		

};
