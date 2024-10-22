// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/*
* En el editor del proyecto se ha creado un canal de colisiones nuevo, por lo que el GameTraceChannel1 ahora esta ocupado.
* Está ocupado por un canal dedicado a las mallas de los personajes.
* Este canal sera de utilidad para saber donde ha golpeado la bala en los Pjs. 
*/

#define ECC_SkeletonMesh ECollisionChannel::ECC_GameTraceChannel1