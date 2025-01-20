// Fill out your copyright notice in the Description page of Project Settings.


#include "NBC_MyPawn.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
ANBC_MyPawn::ANBC_MyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>("Collision");
	CapsuleCollision->InitCapsuleSize(42.f, 97.f);
	RootComponent = CapsuleCollision;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh");
	Mesh->SetupAttachment(RootComponent);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	FollowCamera->bUsePawnControlRotation = false;

	DefaultMoveSpeed = 500.f;
	MoveSpeed = DefaultMoveSpeed;
	RaiseSpeed = 300.f;
	LookSpeed = 100.f;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext>DefaultContext(TEXT("/Game/IMC_Action.IMC_Action"));
	if (DefaultContext.Succeeded())
	{
		MappingContext = DefaultContext.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>InputMove(TEXT("/Game/IA_Move.IA_Move"));
	if (InputMove.Succeeded())
	{
		MoveAction = InputMove.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>InputLook(TEXT("/Game/IA_Look.IA_Look"));
	if (InputLook.Succeeded())
	{
		LookAction = InputLook.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>InputRaise(TEXT("/Game/IA_Raise.IA_Raise"));
	if (InputRaise.Succeeded())
	{
		RaiseAction = InputRaise.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>InputRolling(TEXT("/Game/IA_Rolling.IA_Rolling"));
	if (InputRolling.Succeeded())
	{
		RollingAction = InputRolling.Object;
	}
}

// Called when the game starts or when spawned
void ANBC_MyPawn::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

// Called every frame
void ANBC_MyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!isGround() && !isHovering)
	{
		FVector Gravity = FVector(0.f, 0.f, -980.f);
		FVector GravityOffset = Gravity * DeltaTime;

		AddActorWorldOffset(GravityOffset, true);

		MoveSpeed = FMath::Clamp(MoveSpeed - MoveSpeed * 0.5f * DeltaTime, 0.f, DefaultMoveSpeed);
	}

	if (isGround())
	{
		FRotator Current = GetActorRotation();
		SetActorRotation(FRotator(0.f, Current.Yaw, 0.f));
	}
}

// Called to bind functionality to input
void ANBC_MyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANBC_MyPawn::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANBC_MyPawn::Look);
		EnhancedInputComponent->BindAction(RaiseAction, ETriggerEvent::Triggered, this, &ANBC_MyPawn::Raise);
		EnhancedInputComponent->BindAction(RollingAction, ETriggerEvent::Triggered, this, &ANBC_MyPawn::Rolling);
	}
}

void ANBC_MyPawn::Move(const FInputActionValue& Value)
{
	if (!Controller) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	FVector DeltaLocation = FVector(MovementVector.X, MovementVector.Y, 0.f) * MoveSpeed * GetWorld()->GetDeltaSeconds();

	AddActorLocalOffset(DeltaLocation, true);
}

void ANBC_MyPawn::Look(const FInputActionValue& Value)
{
	if (!Controller) return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	FRotator DeltaRotation = FRotator(LookAxisVector.Y, LookAxisVector.X, 0.f) * LookSpeed * GetWorld()->GetDeltaSeconds();

	AddActorLocalRotation(DeltaRotation);
}

void ANBC_MyPawn::Raise(const FInputActionValue& Value)
{
	if (!Controller) return;

	MoveSpeed = DefaultMoveSpeed;

	float MoveValue = Value.Get<float>();

	isHovering = (MoveValue > 0.f);

	AddActorWorldOffset(FVector(0.f, 0.f, MoveValue * RaiseSpeed * GetWorld()->GetDeltaSeconds()), true);
}

void ANBC_MyPawn::Rolling(const FInputActionValue& Value)
{
	if (!Controller) return;

	float RollValue = Value.Get<float>();

	FRotator DeltaRotation = FRotator(0.f, 0.f, RollValue * LookSpeed * GetWorld()->GetDeltaSeconds());

	AddActorLocalRotation(DeltaRotation);
}

bool ANBC_MyPawn::isGround()
{
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 97.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	if(bHit) MoveSpeed = DefaultMoveSpeed;

	return bHit;
}
