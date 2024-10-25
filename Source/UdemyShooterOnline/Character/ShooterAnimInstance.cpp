// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "UdemyShooterOnline/Weapon/MasterWeapon.h"
#include "UdemyShooterOnline/ShooterTypes/CombatState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UShooterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}
	if (ShooterCharacter == nullptr) return; //Aseguramos si existe el personaje

	FVector Velocity = ShooterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

	//Si es mayor que 0->true si no->false
	bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = ShooterCharacter->IsWeaponEquipped();
	
	EquippedWeapon = ShooterCharacter->GetEquippedWeapon();

	bIsCrouched = ShooterCharacter->bIsCrouched;

	bIsAiming = ShooterCharacter->IsAiming();

	TurningInPlace = ShooterCharacter->GetTurningInPlace();

	bRotateRootBone = ShooterCharacter->ShouldRotateRootBone();

	bElimmed = ShooterCharacter->IsElimmed();

	//Esto nos da la variable global de la rotacion de la camara
	FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
	//Esto nos da la variable global de la rotacion del personaje
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
	//Coge la diferecia entre donde mira la camara y el personaje
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	//Se interpola la rotacion del momento anterior con el actual de forma que haya un cambio suave
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f);

	YawOffset = DeltaRotation.Yaw;

	//Recoge la rotacion del frame anterior
	CharacterRotationLastFrame = CharacterRotation;
	//Recoge la rotacion del frame actual
	CharacterRotation = ShooterCharacter->GetActorRotation();
	//Calcula la diff
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	//Se divide por delta time para que de un valor mas grande
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	//Se clampea el valor
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = ShooterCharacter->GetAO_Yaw();
	AO_Pitch = ShooterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ShooterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

		FVector OutPosition;
		FRotator OutRotation;

		ShooterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		/*
		* Este codigo recoge la rotacion de la mano derecha necesaria para que apunte a donde hace "hit" el arma,
		* De esta forma, el arma apunta siempre a donde golpea la bala
		* Solo se actualiza de manera local ya que no es necesario hacer replicacion para esto
		*/
		
		if (ShooterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - ShooterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 15.f);
		}
	}

	bUseFabrik = ShooterCharacter->GetCombatState() != ECombatState::ECS_Reloading; //Mientras no estamos recargando podemos usar Fabrik (mano atachada al arma)
}
