#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "ChaseGame.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class POGONYA_API AChaseState : public AGameStateBase {
    GENERATED_BODY()
public:
    UPROPERTY(Replicated) float Remaining = 360;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Round") bool bStarted = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Round") bool bFinished = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Round") bool bGateOpen = false;
    UPROPERTY(Replicated) FString Result;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Round") int32 Escaped = 0;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class POGONYA_API AChasePickup : public AActor {
    GENERATED_BODY()
public:
    AChasePickup();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* KeyShaft;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* KeyTooth;
    UPROPERTY(ReplicatedUsing=OnRep_Kind, BlueprintReadWrite, Category="Pickup") bool bKey = false;
    UFUNCTION(BlueprintCallable, Category="Pickup") void OnRep_Kind();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class POGONYA_API AChaseCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AChaseCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook, BlueprintReadWrite, Category="Chase|State") bool bHunter = false;
    UPROPERTY(ReplicatedUsing=RefreshLook, BlueprintReadWrite, Category="Chase|State") bool bStick = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Chase|State") bool bKey = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Chase|State") int32 Lives = 3;
    UPROPERTY(ReplicatedUsing=RefreshLook, BlueprintReadWrite, Category="Chase|State") bool bOut = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Chase|State") bool bEscaped = false;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Chase|State") float LastSwing = -10;
    UPROPERTY(Replicated, BlueprintReadWrite, Category="Chase|State") float LastHit = -10;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float WalkSpeed = 420;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float RunnerSprintSpeed = 570;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float HunterSprintSpeed = 610;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float PickupRange = 190;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float AttackRange = 180;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float AttackCooldown = .8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float HitInvulnerability = 1.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Chase|Tuning") float ExitRange = 260;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") USpringArmComponent* Arm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UCameraComponent* Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") USceneComponent* Figure;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* Mask;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* LeftLeg;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* RightLeg;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* LeftArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* RightArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") UStaticMeshComponent* Weapon;
    UFUNCTION(BlueprintCallable, Category="Chase|Visual") void RefreshLook();
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Chase|Network") void Interact();
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Chase|Network") void Attack();
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Chase|Network") void SetSprint(bool Enabled);
    UFUNCTION(BlueprintCallable, Category="Chase|Combat") void Hurt();
    UFUNCTION(BlueprintPure, Category="Chase|Queries") AChaseState* ChaseState() const;
    UFUNCTION(BlueprintPure, Category="Chase|Queries") bool HasStrikeLine(AChaseCharacter* Target) const;
    UFUNCTION(BlueprintPure, Category="Chase|Queries") FVector ExitPosition() const;
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphMoveForward(float Value);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphMoveRight(float Value);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphLookYaw(float Value);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphLookPitch(float Value);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphSprint(bool Enabled);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphJump(bool Pressed);
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphInteract();
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphAttack();
    UFUNCTION(BlueprintNativeEvent, Category="Chase|Graphs") void GraphReceiveHit();
    bool bBot = false;
    FVector BotTarget;
    float NextDecision = 0;
    void MoveForward(float V);
    void MoveRight(float V);
    void LookYaw(float V);
    void LookPitch(float V);
    void SprintOn();
    void SprintOff();
    void JumpPressed();
    void JumpReleased();
private:
    UStaticMeshComponent* Part(FName Name, USceneComponent* Parent, const TCHAR* Shape, FVector Pos, FVector Scale);
};

UCLASS()
class POGONYA_API AChaseController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void SetupInputComponent() override;
    UFUNCTION(Server, Reliable) void RestartRound();
};

UCLASS()
class POGONYA_API AChaseMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AChaseMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* Player) override;
    virtual void Logout(AController* Player) override;
    virtual void Tick(float Delta) override;
    void StartRound();
    void ResolveRound();
    void RunSelfTest();
    FVector RandomHidingSpot() const;
    static FVector GateLocation() { return FVector(0, 2700, 100); }
private:
    float LobbyTime = 8;
    bool bNeedRosterCheck = false;
};

UCLASS()
class POGONYA_API AChaseHUD : public AHUD {
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

