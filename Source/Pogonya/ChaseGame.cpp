#include "ChaseGame.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/CameraActor.h"
#include "Engine/GameViewportClient.h"

void AChaseState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChaseState, Remaining); DOREPLIFETIME(AChaseState, bStarted);
    DOREPLIFETIME(AChaseState, bFinished); DOREPLIFETIME(AChaseState, bGateOpen);
    DOREPLIFETIME(AChaseState, Result); DOREPLIFETIME(AChaseState, Escaped);
}
AChasePickup::AChasePickup() {
    bReplicates = true; PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup")); SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    KeyShaft=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyShaft")); KeyShaft->SetupAttachment(Mesh);
    KeyTooth=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyTooth")); KeyTooth->SetupAttachment(Mesh);
    for(auto P:{KeyShaft,KeyTooth}) { P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));P->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}
void AChasePickup::OnRep_Kind() {
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,bKey ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetRelativeScale3D(bKey ? FVector(.18f,.18f,.05f) : FVector(.08f,.08f,1.05f));
    KeyShaft->SetVisibility(bKey);KeyTooth->SetVisibility(bKey);
    KeyShaft->SetRelativeLocation(FVector(120,0,0)); KeyShaft->SetRelativeScale3D(FVector(2.2f,.28f,1));
    KeyTooth->SetRelativeLocation(FVector(190,28,0)); KeyTooth->SetRelativeScale3D(FVector(.35f,.6f,1));
    auto Mat = LoadObject<UMaterialInterface>(nullptr,bKey ? TEXT("/Game/Pogonya/Materials/M_Key") : TEXT("/Game/Pogonya/Materials/M_Wood"));
    if(Mat) Mesh->SetMaterial(0,Mat);
    if(Mat){KeyShaft->SetMaterial(0,Mat);KeyTooth->SetMaterial(0,Mat);}
}
void AChasePickup::Tick(float Delta) { Super::Tick(Delta); AddActorLocalRotation(FRotator(0,Delta*32,0)); }
void AChasePickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AChasePickup,bKey); }

UStaticMeshComponent* AChaseCharacter::Part(FName Name, USceneComponent* Parent, const TCHAR* Shape, FVector Pos, FVector Scale) {
    auto P = CreateDefaultSubobject<UStaticMeshComponent>(Name); P->SetupAttachment(Parent);
    P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Shape)); P->SetRelativeLocation(Pos); P->SetRelativeScale3D(Scale);
    P->SetCollisionEnabled(ECollisionEnabled::NoCollision); return P;
}
AChaseCharacter::AChaseCharacter() {
    PrimaryActorTick.bCanEverTick = true; bReplicates = true;
    AIControllerClass=AAIController::StaticClass();
    GetCapsuleComponent()->InitCapsuleSize(34,88);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0,620,0);
    GetCharacterMovement()->MaxWalkSpeed = 420; GetCharacterMovement()->JumpZVelocity = 460;
    GetCharacterMovement()->AirControl = .3f;
    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); Arm->SetupAttachment(RootComponent);
    Arm->TargetArmLength = 320; Arm->SocketOffset = FVector(0,55,65); Arm->bUsePawnControlRotation = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(Arm); Camera->FieldOfView=85;
    Figure = CreateDefaultSubobject<USceneComponent>(TEXT("Figure")); Figure->SetupAttachment(RootComponent); Figure->SetRelativeLocation(FVector(0,0,-88));
    const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* Sphere=TEXT("/Engine/BasicShapes/Sphere.Sphere");
    Body=Part(TEXT("Coat"),Figure,Cube,FVector(0,0,113),FVector(.38f,.55f,.66f));
    Part(TEXT("Hood"),Figure,Sphere,FVector(0,0,159),FVector(.40f,.42f,.46f));
    Mask=Part(TEXT("Mask"),Figure,Sphere,FVector(17,0,160),FVector(.10f,.29f,.27f));
    Part(TEXT("Visor"),Figure,Cube,FVector(22,0,165),FVector(.025f,.26f,.055f));
    LeftLeg=Part(TEXT("LeftLeg"),Figure,Cube,FVector(0,-15,42),FVector(.20f,.22f,.80f));
    RightLeg=Part(TEXT("RightLeg"),Figure,Cube,FVector(0,15,42),FVector(.20f,.22f,.80f));
    LeftArm=Part(TEXT("LeftArm"),Figure,Cube,FVector(0,-36,111),FVector(.20f,.18f,.65f));
    RightArm=Part(TEXT("RightArm"),Figure,Cube,FVector(0,36,111),FVector(.20f,.18f,.65f));
    Part(TEXT("Backpack"),Figure,Cube,FVector(-26,0,116),FVector(.18f,.34f,.43f));
    Weapon=Part(TEXT("Baton"),Figure,Cube,FVector(30,40,85),FVector(.07f,.07f,1.05f));
    Weapon->SetRelativeRotation(FRotator(35,0,0));
}
void AChaseCharacter::BeginPlay() { Super::BeginPlay(); RefreshLook(); }
void AChaseCharacter::RefreshLook() {
    auto Cloth=LoadObject<UMaterialInterface>(nullptr,bHunter?TEXT("/Game/Pogonya/Materials/M_Hunter"):TEXT("/Game/Pogonya/Materials/M_Runner"));
    auto Dark=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Pogonya/Materials/M_Rubber"));
    TArray<UStaticMeshComponent*> Parts; GetComponents(Parts);
    for(auto P:Parts) if(Dark) P->SetMaterial(0,Dark);
    if(Cloth) { Body->SetMaterial(0,Cloth); LeftArm->SetMaterial(0,Cloth); RightArm->SetMaterial(0,Cloth); }
    if(auto M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Pogonya/Materials/M_Mask"))) Mask->SetMaterial(0,M);
    if(auto M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Pogonya/Materials/M_Wood"))) Weapon->SetMaterial(0,M);
    Weapon->SetVisibility(bStick && !bOut);
    GetCapsuleComponent()->SetCollisionEnabled(bOut ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    Figure->SetRelativeRotation(bOut ? FRotator(0,0,85) : FRotator::ZeroRotator);
    Figure->SetRelativeLocation(bOut ? FVector(0,0,-72) : FVector(0,0,-88));
}
void AChaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChaseCharacter,bHunter); DOREPLIFETIME(AChaseCharacter,bStick); DOREPLIFETIME(AChaseCharacter,bKey);
    DOREPLIFETIME(AChaseCharacter,Lives); DOREPLIFETIME(AChaseCharacter,bOut); DOREPLIFETIME(AChaseCharacter,bEscaped);
    DOREPLIFETIME(AChaseCharacter,LastSwing); DOREPLIFETIME(AChaseCharacter,LastHit);
}
void AChaseCharacter::SetupPlayerInputComponent(UInputComponent* I) {
    Super::SetupPlayerInputComponent(I);
    I->BindAxis("MoveForward",this,&AChaseCharacter::MoveForward); I->BindAxis("MoveRight",this,&AChaseCharacter::MoveRight);
    I->BindAxis("Turn",this,&AChaseCharacter::LookYaw); I->BindAxis("LookUp",this,&AChaseCharacter::LookPitch);
    I->BindAction("Jump",IE_Pressed,this,&AChaseCharacter::JumpPressed); I->BindAction("Jump",IE_Released,this,&AChaseCharacter::JumpReleased);
    I->BindAction("Interact",IE_Pressed,this,&AChaseCharacter::Interact); I->BindAction("Attack",IE_Pressed,this,&AChaseCharacter::Attack);
    I->BindAction("Sprint",IE_Pressed,this,&AChaseCharacter::SprintOn); I->BindAction("Sprint",IE_Released,this,&AChaseCharacter::SprintOff);
}
void AChaseCharacter::MoveForward(float V) { GraphMoveForward(V); }
void AChaseCharacter::MoveRight(float V) { GraphMoveRight(V); }
void AChaseCharacter::LookYaw(float V) { GraphLookYaw(V); }
void AChaseCharacter::LookPitch(float V) { GraphLookPitch(V); }
void AChaseCharacter::SetSprint_Implementation(bool Enabled) { GraphSprint(Enabled); }
void AChaseCharacter::Interact_Implementation() { if(HasAuthority()) GraphInteract(); }
void AChaseCharacter::Attack_Implementation() { if(HasAuthority()) GraphAttack(); }
void AChaseCharacter::Hurt() { if(HasAuthority()) GraphReceiveHit(); }
AChaseState* AChaseCharacter::ChaseState() const { return GetWorld()->GetGameState<AChaseState>(); }
FVector AChaseCharacter::ExitPosition() const { return AChaseMode::GateLocation(); }
bool AChaseCharacter::HasStrikeLine(AChaseCharacter* Target) const {
    if(!IsValid(Target)) return false;
    FHitResult Hit; FCollisionQueryParams Query; Query.AddIgnoredActor(this);
    GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Query);
    return !Hit.bBlockingHit || Hit.GetActor()==Target;
}
void AChaseCharacter::GraphMoveForward_Implementation(float V) { if(!bOut && Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V); }
void AChaseCharacter::GraphMoveRight_Implementation(float V) { if(!bOut && Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V); }
void AChaseCharacter::GraphLookYaw_Implementation(float V) { AddControllerYawInput(V); }
void AChaseCharacter::GraphLookPitch_Implementation(float V) { AddControllerPitchInput(V); }
void AChaseCharacter::SprintOn() { GraphSprint(true); SetSprint(true); }
void AChaseCharacter::SprintOff() { GraphSprint(false); SetSprint(false); }
void AChaseCharacter::JumpPressed() { GraphJump(true); }
void AChaseCharacter::JumpReleased() { GraphJump(false); }
void AChaseCharacter::GraphJump_Implementation(bool Pressed) { if(Pressed && !bOut) Jump(); else StopJumping(); }
void AChaseCharacter::GraphSprint_Implementation(bool Enabled) { GetCharacterMovement()->MaxWalkSpeed = Enabled ? (bHunter?610:570) : 420; }
void AChaseCharacter::GraphInteract_Implementation() {
    auto S=GetWorld()->GetGameState<AChaseState>(); if(!S || !S->bStarted || S->bFinished || bOut) return;
    for(TActorIterator<AChasePickup> It(GetWorld());It;++It) {
        if(FVector::Dist(GetActorLocation(),It->GetActorLocation())<190 && ((It->bKey && !bHunter && !bKey) || (!It->bKey && bHunter && !bStick))) {
            if(It->bKey) bKey=true; else bStick=true; It->Destroy(); RefreshLook(); break;
        }
    }
    if(!bHunter && FVector::Dist2D(GetActorLocation(),AChaseMode::GateLocation())<260) {
        if(bKey) S->bGateOpen=true;
        if(S->bGateOpen) { bEscaped=true; bOut=true; S->Escaped++; GetCharacterMovement()->StopMovementImmediately(); RefreshLook(); }
    }
}
void AChaseCharacter::GraphAttack_Implementation() {
    auto S=GetWorld()->GetGameState<AChaseState>(); float Now=GetWorld()->GetTimeSeconds();
    if(!S || !S->bStarted || S->bFinished || bOut || !bHunter || !bStick || Now-LastSwing<.8f) return;
    LastSwing=Now;
    for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It) {
        if(It->bHunter || It->bOut) continue;
        FVector D=It->GetActorLocation()-GetActorLocation();
        if(D.Size()>180 || FVector::DotProduct(D.GetSafeNormal(),GetActorForwardVector())<.25f) continue;
        FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(this);
        GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),It->GetActorLocation(),ECC_Visibility,Q);
        if(!Hit.bBlockingHit || Hit.GetActor()==*It) It->Hurt();
    }
}
void AChaseCharacter::GraphReceiveHit_Implementation() {
    if(!HasAuthority() || bOut || GetWorld()->GetTimeSeconds()-LastHit<1.2f) return;
    LastHit=GetWorld()->GetTimeSeconds(); Lives=FMath::Max(0,Lives-1);
    if(!Lives) {
        bOut=true; GetCharacterMovement()->StopMovementImmediately();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if(bKey) { auto P=GetWorld()->SpawnActor<AChasePickup>(GetActorLocation()+FVector(0,0,-20),FRotator::ZeroRotator); if(P){P->bKey=true;P->OnRep_Kind();} bKey=false; }
        RefreshLook();
    }
}
void AChaseCharacter::Tick(float Delta) {
    Super::Tick(Delta);
    auto S=GetWorld()->GetGameState<AChaseState>(); if(!S) return;
    if(!HasAuthority() && IsLocallyControlled() && FParse::Param(FCommandLine::Get(),TEXT("PogonyaNetworkClient"))){
        static int TestStage=0;static float Before=0,Sent=0;
        float LocalTime=GetWorld()->GetTimeSeconds();
        if(TestStage==0 && LocalTime>12 && S->bStarted){
            Before=S->Remaining;Sent=LocalTime;TestStage=1;
            if(auto PC=Cast<AChaseController>(Controller))PC->RestartRound();
        }
        if(TestStage==1 && LocalTime-Sent>3){
            TestStage=2;int Count=0,Hunters=0;
            for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It){Count++;if(It->bHunter)Hunters++;}
            bool Passed=Count==4 && Hunters==1 && S->bStarted && S->Remaining>Before && Lives==3;
            FString Report=FString::Printf(TEXT("%s: replicated roster=%d hunters=%d; client RPC restarted round; timer before=%.1f after=%.1f; lives=%d\n"),Passed?TEXT("PASS"):TEXT("FAIL"),Count,Hunters,Before,S->Remaining,Lives);
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("PogonyaNetworkTest.txt")));
            FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
        }
    }
    if(!bOut) {
        float T=S->GetServerWorldTimeSeconds(); float Swing=FMath::Sin(T*11)*FMath::Clamp(GetVelocity().Size2D()/500.f,0.f,1.f)*28;
        LeftLeg->SetRelativeRotation(FRotator(Swing,0,0)); RightLeg->SetRelativeRotation(FRotator(-Swing,0,0));
        LeftArm->SetRelativeRotation(FRotator(-Swing,0,0)); RightArm->SetRelativeRotation(FRotator(T-LastSwing<.35f ? -95+260*(T-LastSwing) : Swing,0,0));
        Weapon->SetRelativeRotation(FRotator(T-LastSwing<.35f ? -75+360*(T-LastSwing) : 35,0,0));
    }
    if(!HasAuthority() || !bBot || bOut || !S->bStarted || S->bFinished) return;
    float Now=GetWorld()->GetTimeSeconds();
    if(Now>NextDecision) {
        NextDecision=Now+.35f;
        AChaseCharacter* Threat=nullptr; float Best=BIG_NUMBER; bool Found=false;
        for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It) if(It->bHunter!=bHunter && !It->bOut) {
            float D=FVector::DistSquared(GetActorLocation(),It->GetActorLocation()); if(D<Best){Best=D;Threat=*It;}
        }
        if(bHunter && bStick && Threat) { BotTarget=Threat->GetActorLocation(); Found=true; }
        if(!bHunter && (bKey || S->bGateOpen)) { BotTarget=AChaseMode::GateLocation(); Found=true; }
        if(!Found) for(TActorIterator<AChasePickup> It(GetWorld());It;++It) if(It->bKey!=bHunter){ BotTarget=It->GetActorLocation(); Found=true; break; }
        if(!bHunter && Threat && Best<FMath::Square(650.f)) { BotTarget=GetActorLocation()+(GetActorLocation()-Threat->GetActorLocation()).GetSafeNormal()*700; Found=true; }
        if(!Found && FVector::Dist2D(GetActorLocation(),BotTarget)<150) BotTarget=FVector(FMath::FRandRange(-1900.f,1900.f),FMath::FRandRange(-2300.f,2300.f),100);
        Interact(); Attack(); SetSprint(true);
    }
    FVector Desired=(BotTarget-GetActorLocation()).GetSafeNormal2D(); FVector Steer=Desired;
    FCollisionQueryParams Q; Q.AddIgnoredActor(this);
    float Score=-BIG_NUMBER;
    for(float Angle : {0.f,35.f,-35.f,70.f,-70.f,110.f,-110.f,160.f}) {
        FVector D=Desired.RotateAngleAxis(Angle,FVector::UpVector); FHitResult H;
        bool Block=GetWorld()->SweepSingleByChannel(H,GetActorLocation(),GetActorLocation()+D*200,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(40),Q);
        float Value=(Block ? H.Time*.7f : 1.5f)+FVector::DotProduct(D,Desired)*.5f;
        if(Value>Score){Score=Value;Steer=D;}
    }
    AddMovementInput(Steer,1);
}

void AChaseController::SetupInputComponent(){Super::SetupInputComponent();InputComponent->BindAction("Restart",IE_Pressed,this,&AChaseController::RestartRound);}
void AChaseController::RestartRound_Implementation(){if(auto M=GetWorld()->GetAuthGameMode<AChaseMode>()) M->StartRound();}
AChaseMode::AChaseMode(){DefaultPawnClass=AChaseCharacter::StaticClass();PlayerControllerClass=AChaseController::StaticClass();GameStateClass=AChaseState::StaticClass();HUDClass=AChaseHUD::StaticClass();PrimaryActorTick.bCanEverTick=true;}
void AChaseMode::BeginPlay(){Super::BeginPlay();}
void AChaseMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage){
    if(auto BlueprintPawn=LoadClass<APawn>(nullptr,TEXT("/Game/Pogonya/Characters/BP_ChaseCharacter_Graphs.BP_ChaseCharacter_Graphs_C"))) DefaultPawnClass=BlueprintPawn;
    Super::InitGame(MapName,Options,ErrorMessage);
}
void AChaseMode::PostLogin(APlayerController* P){Super::PostLogin(P);bNeedRosterCheck=true;}
void AChaseMode::Logout(AController* P){ if(auto C=Cast<AChaseCharacter>(P->GetPawn())){if(C->bKey){auto K=GetWorld()->SpawnActor<AChasePickup>(C->GetActorLocation(),FRotator::ZeroRotator);if(K){K->bKey=true;K->OnRep_Kind();}}} Super::Logout(P); bNeedRosterCheck=true; }
FVector AChaseMode::RandomHidingSpot() const {
    static const FVector Spots[] = {FVector(-1770,-1850,80),FVector(1740,1770,80),FVector(-1770,1330,80),FVector(1750,-950,80),FVector(200,1480,80),FVector(-100,-1400,80),FVector(1170,200,80),FVector(-1150,400,80)};
    return Spots[FMath::RandRange(0,7)];
}
void AChaseMode::StartRound(){
    auto S=GetGameState<AChaseState>(); if(!S)return;
    for(TActorIterator<AChasePickup> It(GetWorld());It;++It)It->Destroy();
    TArray<AChaseCharacter*> Players;
    for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It)if(!It->bBot && It->GetController())Players.Add(*It);else It->Destroy();
    for(int i=Players.Num();i<4;i++){
        auto C=GetWorld()->SpawnActor<AChaseCharacter>(DefaultPawnClass,FVector(0,-2300,100),FRotator::ZeroRotator);
        if(C){ C->bBot=true; C->SpawnDefaultController(); Players.Add(C); }
    }
    if(Players.IsEmpty())return;
    int Hunter=FMath::RandRange(0,Players.Num()-1);
    for(int i=0;i<Players.Num();i++){
        auto C=Players[i]; C->bHunter=i==Hunter; C->bStick=false; C->bKey=false; C->Lives=3; C->bOut=false; C->bEscaped=false;
        C->LastHit=-10; C->LastSwing=-10; C->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        C->TeleportTo(FVector(-600+i*350,-2250,100),FRotator(0,90,0)); C->GetCharacterMovement()->SetMovementMode(MOVE_Walking); C->RefreshLook();
        if(C->GetController())C->GetController()->SetControlRotation(FRotator(-10,90,0));
    }
    FVector KeySpot=RandomHidingSpot(),StickSpot=RandomHidingSpot(); while(StickSpot==KeySpot)StickSpot=RandomHidingSpot();
    auto Key=GetWorld()->SpawnActor<AChasePickup>(KeySpot,FRotator::ZeroRotator);if(Key){Key->bKey=true;Key->OnRep_Kind();}
    auto Stick=GetWorld()->SpawnActor<AChasePickup>(StickSpot,FRotator::ZeroRotator);if(Stick){Stick->bKey=false;Stick->OnRep_Kind();}
    S->bStarted=true;S->bFinished=false;S->bGateOpen=false;S->Escaped=0;S->Remaining=360;S->Result.Empty();
}
void AChaseMode::ResolveRound(){
    auto S=GetGameState<AChaseState>();if(!S || S->bFinished)return;
    int Active=0,Hunters=0;
    for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It)if(!It->bOut){if(It->bHunter)Hunters++;else Active++;}
    if(Active==0 || Hunters==0 || S->Remaining<=0){S->bFinished=true;S->Result = Hunters==0 ? TEXT("Гонщик покинул раунд") : S->Escaped>0 ? TEXT("Побег удался") : TEXT("Гонщик победил");}
}
void AChaseMode::Tick(float Delta){
    Super::Tick(Delta);auto S=GetGameState<AChaseState>();if(!S)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaNetworkServer")) && GetWorld()->GetTimeSeconds()>40) FPlatformMisc::RequestExit(false);
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaPreview"))){
        static int Stage=0;static ACameraActor* PreviewCamera=nullptr;float T=GetWorld()->GetTimeSeconds();
        if(Stage==0 && T>2){PreviewCamera=GetWorld()->SpawnActor<ACameraActor>(FVector(50,-2450,245),FRotator(-3,90,0));if(auto P=GetWorld()->GetFirstPlayerController()){P->SetViewTarget(PreviewCamera);if(P->GetHUD())P->GetHUD()->bShowHUD=false;}Stage=1;}
        if(Stage==1 && T>12){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Pogonya_Courtyard.png"),false,false);Stage=2;}
        if(Stage==2 && T>14){PreviewCamera->SetActorLocationAndRotation(FVector(3600,-4200,3500),FRotator(-34,130,0));Stage=3;}
        if(Stage==3 && T>22){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Pogonya_Overview.png"),false,false);Stage=4;}
        if(Stage==4 && T>25)FPlatformMisc::RequestExit(false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaSelfTest")) && GetWorld()->GetTimeSeconds()>2){ RunSelfTest(); return; }
    if(!S->bStarted){LobbyTime-=Delta;S->Remaining=FMath::Max(0.f,LobbyTime);if(LobbyTime<=0)StartRound();return;}
    if(bNeedRosterCheck){bNeedRosterCheck=false;int Humans=GetNumPlayers();int Bots=0;for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It)if(It->bBot)Bots++;if(Humans+Bots>4){for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It)if(It->bBot && !It->bHunter && !It->bKey){It->Destroy();break;}}}
    if(!S->bFinished){S->Remaining=FMath::Max(0.f,S->Remaining-Delta);ResolveRound();}
}

void AChaseMode::RunSelfTest(){
    static bool Ran=false;if(Ran)return;Ran=true;
    FString Report;int Failures=0;
    auto Check=[&](bool Passed,const TCHAR* Name){Report+=FString(Passed?TEXT("PASS "):TEXT("FAIL "))+Name+TEXT("\n");if(!Passed)Failures++;};
    StartRound();auto S=GetGameState<AChaseState>();
    AChaseCharacter* Hunter=nullptr;TArray<AChaseCharacter*> Runners;int Total=0;
    for(TActorIterator<AChaseCharacter> It(GetWorld());It;++It){It->bBot=false;It->GetCharacterMovement()->StopMovementImmediately();Total++;if(It->bHunter)Hunter=*It;else Runners.Add(*It);}
    Check(Total==4 && Hunter && Runners.Num()==3,TEXT("One randomly selected hunter, three runners"));
    if(Hunter && Runners.Num()==3){
        auto Runner=Runners[0];
        Check(Hunter->GetClass()->GetName()==TEXT("BP_ChaseCharacter_Graphs_C"),TEXT("Game spawns the Blueprint graph character"));
        bool GraphOverrides=true;
        for(const TCHAR* Name:{TEXT("GraphMoveForward"),TEXT("GraphMoveRight"),TEXT("GraphLookYaw"),TEXT("GraphLookPitch"),TEXT("GraphSprint"),TEXT("GraphJump"),TEXT("GraphInteract"),TEXT("GraphAttack"),TEXT("GraphReceiveHit")}){
            auto F=Hunter->FindFunction(FName(Name));GraphOverrides=GraphOverrides && F && F->Script.Num()>0;
        }
        Check(GraphOverrides,TEXT("All nine character mechanics execute Blueprint bytecode"));
        Runner->RunnerSprintSpeed=555;Runner->SetSprint(true);
        Check(Runner->GetCharacterMovement()->MaxWalkSpeed==555,TEXT("Sprint graph uses editable runner speed"));
        Runner->SetSprint(false);Check(Runner->GetCharacterMovement()->MaxWalkSpeed==Runner->WalkSpeed,TEXT("Sprint release graph restores walk speed"));
        Hunter->HunterSprintSpeed=605;Hunter->SetSprint(true);
        Check(Hunter->GetCharacterMovement()->MaxWalkSpeed==605,TEXT("Sprint graph uses editable hunter speed"));
        Hunter->ConsumeMovementInputVector();
        if(Hunter->GetController())Hunter->GetController()->SetControlRotation(FRotator(0,90,0));
        Hunter->GraphMoveForward(1);
        Check(Hunter->ConsumeMovementInputVector().Y>.9f,TEXT("Movement graph follows camera forward yaw"));
        Hunter->GraphMoveRight(1);
        Check(Hunter->ConsumeMovementInputVector().X<-.9f,TEXT("Movement graph follows camera right yaw"));
        Hunter->TeleportTo(FVector(0,-2150,100),FRotator::ZeroRotator);
        Runner->TeleportTo(FVector(110,-2150,100),FRotator::ZeroRotator);
        Runners[1]->TeleportTo(FVector(-900,-2400,100),FRotator::ZeroRotator);
        Runners[2]->TeleportTo(FVector(900,-2400,100),FRotator::ZeroRotator);
        Hunter->Attack();Check(Runner->Lives==3,TEXT("Cannot attack without stick"));
        AChasePickup* Key=nullptr;AChasePickup* Stick=nullptr;
        for(TActorIterator<AChasePickup> It(GetWorld());It;++It){if(It->bKey)Key=*It;else Stick=*It;}
        Check(Key && Stick && Key->GetActorLocation()!=Stick->GetActorLocation(),TEXT("Key and stick in distinct hiding places"));
        if(Key && Stick){
            Key->SetActorLocation(Hunter->GetActorLocation());Hunter->Interact();Check(!Hunter->bKey,TEXT("Hunter cannot pick up key"));
            Stick->SetActorLocation(Runner->GetActorLocation());Runner->Interact();Check(!Runner->bStick,TEXT("Runner cannot pick up stick"));
            Hunter->Interact();Check(Hunter->bStick,TEXT("Hunter picks up stick"));
            Runner->Interact();Check(Runner->bKey,TEXT("Runner picks up key"));
        }
        Hunter->AttackRange=50;Hunter->Attack();Check(Runner->Lives==3,TEXT("Attack graph respects editable hit distance"));Hunter->AttackRange=180;Hunter->LastSwing=-10;
        Hunter->Attack();Check(Runner->Lives==2,TEXT("First hit removes exactly one life"));
        Hunter->Attack();Check(Runner->Lives==2,TEXT("Attack cooldown prevents repeated damage"));
        Runner->Hurt();Check(Runner->Lives==2,TEXT("Damage graph respects hit invulnerability"));
        for(int i=0;i<2;i++){Hunter->LastSwing=-10;Runner->LastHit=-10;Hunter->Attack();}
        Check(Runner->Lives==0 && Runner->bOut,TEXT("Third hit eliminates runner"));
        bool Dropped=false;for(TActorIterator<AChasePickup> It(GetWorld());It;++It)if(It->bKey && FVector::Dist(It->GetActorLocation(),Runner->GetActorLocation())<100)Dropped=true;
        Check(Dropped && !Runner->bKey,TEXT("Eliminated carrier drops key"));
        auto Survivor=Runners[1];Survivor->TeleportTo(GateLocation(),FRotator::ZeroRotator);Survivor->Interact();
        Check(!S->bGateOpen && !Survivor->bEscaped,TEXT("Gate requires key"));
        Survivor->bKey=true;Survivor->Interact();Check(S->bGateOpen && Survivor->bEscaped && S->Escaped==1,TEXT("Key opens gate and carrier escapes"));
        Runners[2]->TeleportTo(GateLocation()+FVector(120,0,0),FRotator::ZeroRotator);Runners[2]->Interact();
        Check(Runners[2]->bEscaped && S->Escaped==2,TEXT("Other runners can use unlocked exit"));
        ResolveRound();Check(S->bFinished,TEXT("Round resolves after all runners leave or die"));
        StartRound();Check(!S->bFinished && !S->bGateOpen && S->Escaped==0 && S->Remaining==360,TEXT("New round resets state"));
        S->Remaining=0;ResolveRound();Check(S->bFinished,TEXT("Timeout finishes round"));
    }
    Report+=FString::Printf(TEXT("Failures: %d\n"),Failures);
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("PogonyaSelfTest.txt")));
    UE_LOG(LogTemp,Display,TEXT("POGONYA SELF TEST\n%s"),*Report);
    FGenericPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
}

void AChaseHUD::DrawHUD(){
    Super::DrawHUD(); if(!Canvas)return;auto S=GetWorld()->GetGameState<AChaseState>(); auto C=Cast<AChaseCharacter>(GetOwningPawn());if(!S || !C)return;
    float W=Canvas->SizeX,H=Canvas->SizeY;
    auto Text=[&](const FString& T,float X,float Y,FLinearColor Color,float Scale=1.f){DrawText(T,Color,X,Y,GEngine->GetMediumFont(),Scale);};
    DrawRect(FLinearColor(.015f,.025f,.03f,.88f),28,26,420,148);
    DrawRect(FLinearColor(.96f,.49f,.13f,1),28,26,4,148);
    Text(TEXT("ПОГОНЯ / СЕКТОР 04"),48,40,FLinearColor(.95f,.65f,.27f),1.15f);
    Text(C->bHunter?TEXT("ВЫ — ГОНЩИК"):TEXT("ВЫ — БЕГЛЕЦ"),48,74,C->bHunter?FLinearColor(1,.3f,.2f):FLinearColor(.3f,.85f,.8f));
    FString Task=C->bHunter?(C->bStick?TEXT("Догоните беглецов. ЛКМ — удар"):TEXT("Найдите палку. E — подобрать")):(C->bKey?TEXT("Ключ найден. Идите к воротам"):S->bGateOpen?TEXT("Ворота открыты. Бегите к выходу"):TEXT("Найдите ключ и откройте ворота"));
    Text(Task,48,108,FLinearColor::White,.85f);
    Text(FString::Printf(TEXT("ЖИЗНИ  %d / 3     СПАСЛИСЬ  %d"),C->Lives,S->Escaped),48,143,FLinearColor(.7f,.76f,.77f),.8f);
    int Seconds=FMath::CeilToInt(S->Remaining);
    Text(FString::Printf(TEXT("%02d:%02d"),Seconds/60,Seconds%60),W-125,40,FLinearColor::White,1.5f);
    DrawRect(FLinearColor(.015f,.025f,.03f,.8f),28,H-60,W-56,35);
    Text(TEXT("WASD — движение   Мышь — обзор   Shift — бег   Пробел — прыжок   E — действие   ЛКМ — удар   R — новый раунд"),43,H-52,FLinearColor(.7f,.76f,.77f),.75f);
    DrawRect(FLinearColor(1,1,1,.7f),W/2-2,H/2-2,4,4);
    if(!S->bStarted)Text(TEXT("Подготовка раунда. Ожидание игроков…"),W/2-220,H/2-70,FLinearColor::White,1.1f);
    if(S->bFinished || C->bOut){
        DrawRect(FLinearColor(.01f,.02f,.025f,.88f),W/2-260,H/2-65,520,115);
        Text(S->bFinished?S->Result:C->bEscaped?TEXT("ВЫ ВЫБРАЛИСЬ"):TEXT("ВЫ ВЫБЫЛИ"),W/2-220,H/2-48,FLinearColor(.95f,.65f,.27f),1.4f);
        Text(TEXT("R — начать новый раунд"),W/2-220,H/2+3,FLinearColor::White);
    }
    if(!C->bOut && S->bStarted && !S->bFinished){
        for(TActorIterator<AChasePickup> It(GetWorld());It;++It)if(FVector::Dist(C->GetActorLocation(),It->GetActorLocation())<190 && It->bKey!=C->bHunter)Text(It->bKey?TEXT("[E] ПОДОБРАТЬ КЛЮЧ"):TEXT("[E] ПОДОБРАТЬ ПАЛКУ"),W/2-120,H*.65f,FLinearColor(.95f,.65f,.27f));
        if(!C->bHunter && FVector::Dist2D(C->GetActorLocation(),AChaseMode::GateLocation())<260)Text(S->bGateOpen?TEXT("[E] ПОКИНУТЬ СЕКТОР"):C->bKey?TEXT("[E] ОТКРЫТЬ И ВЫЙТИ"):TEXT("НУЖЕН КЛЮЧ"),W/2-140,H*.65f,FLinearColor(.95f,.65f,.27f));
    }
    if(S->GetServerWorldTimeSeconds()-C->LastHit<.25f)DrawRect(FLinearColor(.8f,0,0,.25f),0,0,W,H);
}



