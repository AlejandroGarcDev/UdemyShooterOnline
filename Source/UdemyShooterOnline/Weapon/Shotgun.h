#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class UDEMYSHOOTERONLINE_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()
	
public:

	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);

	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets);

private:

	//numero de perdigones que dispara la escopeta por disparo
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
