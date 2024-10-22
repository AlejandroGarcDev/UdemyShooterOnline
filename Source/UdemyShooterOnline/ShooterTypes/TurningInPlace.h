#pragma once

UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	//Enum Turning In Place --> Enum para marcar si tiene que girar el personaje cuando esta quieto y mira para los lados
	ETIP_Left UMETA(DisplayName = "Turning Left"),
	ETIP_Right UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),
	ETIP_MAX UMETA(DisplayName = "Turning Left")
};