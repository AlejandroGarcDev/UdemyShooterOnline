// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MasterWeapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AFlag : public AMasterWeapon
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere)
	FVector InitialPosition = FVector(0.f,0.f,0.f);
private:

	AFlag();

	virtual void Dropped() override;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* FlagMesh;

protected:
	virtual void OnEquipped() override;
	virtual void OnDropped() override;
};
