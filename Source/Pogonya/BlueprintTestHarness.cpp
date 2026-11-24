#include "BlueprintTestHarness.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
FDelegateHandle BlueprintTestTickHandle;
bool Done=false;
int Failures=0;
FString Report;
UClass *CharacterClass=nullptr,*PickupClass=nullptr;
bool Bool(UObject* O,const TCHAR* N){auto P=O?FindFProperty<FBoolProperty>(O->GetClass(),N):nullptr;return P&&P->GetPropertyValue_InContainer(O);}
void SetBool(UObject* O,const TCHAR* N,bool V){if(auto P=FindFProperty<FBoolProperty>(O->GetClass(),N))P->SetPropertyValue_InContainer(O,V);}
int Number(UObject* O,const TCHAR* N){auto P=FindFProperty<FIntProperty>(O->GetClass(),N);return P?P->GetPropertyValue_InContainer(O):-999;}
void SetInt(UObject* O,const TCHAR* N,int V){if(auto P=FindFProperty<FIntProperty>(O->GetClass(),N))P->SetPropertyValue_InContainer(O,V);}
float Real(UObject* O,const TCHAR* N){auto P=FindFProperty<FFloatProperty>(O->GetClass(),N);return P?P->GetPropertyValue_InContainer(O):-999;}
void SetReal(UObject* O,const TCHAR* N,float V){if(auto P=FindFProperty<FFloatProperty>(O->GetClass(),N))P->SetPropertyValue_InContainer(O,V);}
void Call(UObject* O,const TCHAR* N){if(auto F=O?O->FindFunction(N):nullptr)O->ProcessEvent(F,nullptr);else {Report+=FString(TEXT("FAIL missing event "))+N+TEXT("\n");++Failures;}}
void AdvanceGate(AActor* G){for(int i=0;i<100;i++){float Delta=1.f/30.f;if(auto F=G->FindFunction(TEXT("ReceiveTick")))G->ProcessEvent(F,&Delta);}}
void Check(bool Good,const TCHAR* N){Report+=FString(Good?TEXT("PASS "):TEXT("FAIL "))+N+TEXT("\n");if(!Good)++Failures;}
TArray<AActor*> Actors(UWorld* W,UClass* C){TArray<AActor*> R;for(TActorIterator<AActor> I(W,C);I;++I)if(IsValid(*I))R.Add(*I);return R;}
void Freeze(UWorld* W){for(auto A:Actors(W,CharacterClass)){SetBool(A,TEXT("bBot"),false);if(auto C=Cast<ACharacter>(A))C->GetCharacterMovement()->StopMovementImmediately();}}
void Finish(const TCHAR* Name){Report+=FString::Printf(TEXT("Failures: %d\n"),Failures);FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/Name));UE_LOG(LogTemp,Display,TEXT("%s"),*Report);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);}
void Test(UWorld* W){
    Done=true;auto Mode=W->GetAuthGameMode();auto State=W->GetGameState();
    Check(Mode&&Mode->GetClass()->GetName()==TEXT("BP_PogonyaGameMode_C"),TEXT("Pure Blueprint GameMode is active"));
    if(!Mode||Mode->GetClass()->GetName()!=TEXT("BP_PogonyaGameMode_C")){Finish(TEXT("PureBlueprintSelfTest.txt"));return;}
    Call(Mode,TEXT("StartRound"));auto Roster=Actors(W,CharacterClass);AActor* Hunter=nullptr;TArray<AActor*> Runners;
    for(auto A:Roster)if(Bool(A,TEXT("bHunter")))Hunter=A;else Runners.Add(A);
    Check(Roster.Num()==4&&Hunter&&Runners.Num()==3,TEXT("Four players and exactly one random hunter"));
    Check(State&&Bool(State,TEXT("bStarted"))&&!Bool(State,TEXT("bFinished")),TEXT("Round starts through Blueprint event"));
    Check(State&&Real(State,TEXT("Remaining"))==360,TEXT("Round timer is 360 seconds"));
    bool Three=true;for(auto A:Roster)Three&=Number(A,TEXT("Lives"))==3;Check(Three,TEXT("StartRound sets Lives to three"));
    if(!Hunter||Runners.Num()!=3){Finish(TEXT("PureBlueprintSelfTest.txt"));return;}
    Freeze(W);auto Runner=Runners[0];auto Survivor=Runners[1];auto Last=Runners[2];
    Hunter->SetActorLocationAndRotation(FVector(0,-2250,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
    Runner->SetActorLocation(FVector(150,-2250,100),false,nullptr,ETeleportType::TeleportPhysics);
    Survivor->SetActorLocation(FVector(1500,-2250,100));Last->SetActorLocation(FVector(-1500,-2250,100));
    Check(CharacterClass->GetSuperClass()==ACharacter::StaticClass(),TEXT("Character directly inherits engine Character, no project C++ parent"));
    Check(PickupClass->GetSuperClass()==AActor::StaticClass(),TEXT("Pickup directly inherits engine Actor"));
    auto Items=Actors(W,PickupClass);AActor *Key=nullptr,*Stick=nullptr;for(auto A:Items)if(Bool(A,TEXT("bKey")))Key=A;else Stick=A;
    Check(Items.Num()==2&&Key&&Stick,TEXT("One key and one stick spawned"));
    Check(Key&&Stick&&FVector::Dist(Key->GetActorLocation(),Stick->GetActorLocation())>100,TEXT("Different random hiding spots"));
    if(!Key||!Stick){Finish(TEXT("PureBlueprintSelfTest.txt"));return;}
    Call(Hunter,TEXT("GraphAttack"));Check(Number(Runner,TEXT("Lives"))==3,TEXT("Attack without stick is rejected"));
    Key->SetActorLocation(Hunter->GetActorLocation()+FVector(40,0,0));Stick->SetActorLocation(FVector(1500,0,100));
    Call(Hunter,TEXT("GraphInteract"));Check(!Bool(Hunter,TEXT("bKey"))&&IsValid(Key),TEXT("Hunter cannot collect key"));
    Call(Runner,TEXT("GraphInteract"));Check(Bool(Runner,TEXT("bKey"))&&!IsValid(Key),TEXT("Runner collects key and removes pickup"));
    Stick->SetActorLocation(Runner->GetActorLocation()+FVector(20,0,0));Call(Runner,TEXT("GraphInteract"));Check(!Bool(Runner,TEXT("bStick"))&&IsValid(Stick),TEXT("Runner cannot collect stick"));
    Call(Hunter,TEXT("GraphInteract"));Check(Bool(Hunter,TEXT("bStick"))&&!IsValid(Stick),TEXT("Hunter collects stick"));
    SetReal(Hunter,TEXT("AttackRange"),50);Call(Hunter,TEXT("GraphAttack"));Check(Number(Runner,TEXT("Lives"))==3,TEXT("Editable attack range is respected"));SetReal(Hunter,TEXT("AttackRange"),180);SetReal(Hunter,TEXT("LastSwing"),-10);
    auto Wall=W->SpawnActor<AStaticMeshActor>(FVector(75,-2250,100),FRotator::ZeroRotator);auto Mesh=Wall->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetWorldScale3D(FVector(.15,2,2));Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Call(Hunter,TEXT("GraphAttack"));Check(Number(Runner,TEXT("Lives"))==3,TEXT("Wall blocks stick attack"));Wall->Destroy();SetReal(Hunter,TEXT("LastSwing"),-10);
    Call(Hunter,TEXT("GraphAttack"));Check(Number(Runner,TEXT("Lives"))==2,TEXT("One hit removes exactly one life"));Call(Hunter,TEXT("GraphAttack"));Check(Number(Runner,TEXT("Lives"))==2,TEXT("Attack cooldown blocks immediate repeated swing"));
    Call(Runner,TEXT("GraphReceiveHit"));Check(Number(Runner,TEXT("Lives"))==2,TEXT("Temporary invulnerability blocks repeated damage"));
    for(int i=0;i<2;i++){SetReal(Hunter,TEXT("LastSwing"),-10);SetReal(Runner,TEXT("LastHit"),-10);Call(Hunter,TEXT("GraphAttack"));}
    Check(Number(Runner,TEXT("Lives"))==0&&Bool(Runner,TEXT("bOut")),TEXT("Third hit eliminates runner"));
    Check(Cast<ACharacter>(Runner)->GetCharacterMovement()->MovementMode==MOVE_None,TEXT("Eliminated character stops without falling through the floor"));
    Check(!Bool(Runner,TEXT("bKey"))&&Actors(W,PickupClass).Num()==1,TEXT("Elimination drops carried key"));
    Survivor->SetActorLocation(FVector(0,2700,100));Call(Survivor,TEXT("GraphInteract"));Check(!Bool(State,TEXT("bGateOpen"))&&!Bool(Survivor,TEXT("bEscaped")),TEXT("Exit remains locked without key"));
    SetBool(Survivor,TEXT("bKey"),true);Call(Survivor,TEXT("GraphInteract"));Check(Bool(State,TEXT("bGateOpen"))&&Bool(Survivor,TEXT("bEscaped"))&&Number(State,TEXT("Escaped"))==1,TEXT("Key unlocks shared exit and carrier escapes"));
    Last->SetActorLocation(FVector(0,2700,100));Call(Last,TEXT("GraphInteract"));Check(Bool(Last,TEXT("bEscaped"))&&Number(State,TEXT("Escaped"))==2,TEXT("Second runner uses already open exit without key"));
    auto Gates=Actors(W,LoadClass<AActor>(nullptr,TEXT("/Game/Pogonya/Blueprints/BP_PogonyaGate.BP_PogonyaGate_C")));for(auto G:Gates)AdvanceGate(G);Check(Gates.Num()==1&&Real(Gates[0],TEXT("OpenAmount"))>.99f,TEXT("Blueprint gate opens smoothly"));
    Call(Mode,TEXT("ResolveRound"));Check(Bool(State,TEXT("bFinished")),TEXT("Round ends after all runners escape or are eliminated"));
    Call(Mode,TEXT("StartRound"));Check(!Bool(State,TEXT("bFinished"))&&!Bool(State,TEXT("bGateOpen"))&&Number(State,TEXT("Escaped"))==0&&Real(State,TEXT("Remaining"))==360,TEXT("Restart resets round state"));
    for(auto G:Gates)AdvanceGate(G);Check(Gates.Num()==1&&Real(Gates[0],TEXT("OpenAmount"))<.01f,TEXT("Blueprint gate closes after restart"));
    Three=true;for(auto A:Actors(W,CharacterClass))Three&=Number(A,TEXT("Lives"))==3;Check(Three,TEXT("Restart restores three lives"));
    SetReal(State,TEXT("Remaining"),0);Call(Mode,TEXT("ResolveRound"));Check(Bool(State,TEXT("bFinished")),TEXT("Time limit ends round"));
    Finish(TEXT("PureBlueprintSelfTest.txt"));
}
void Tick(UWorld* W,ELevelTick,float){
    if(!W||!W->IsGameWorld())return;
    if(!CharacterClass){CharacterClass=LoadClass<AActor>(nullptr,TEXT("/Game/Pogonya/Blueprints/BP_PogonyaCharacter.BP_PogonyaCharacter_C"));PickupClass=LoadClass<AActor>(nullptr,TEXT("/Game/Pogonya/Blueprints/BP_PogonyaPickup.BP_PogonyaPickup_C"));if(CharacterClass)CharacterClass->AddToRoot();if(PickupClass)PickupClass->AddToRoot();}
    if(!CharacterClass||!PickupClass)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintSelfTest"))&&!Done&&W->GetTimeSeconds()>3)Test(W);
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintServer"))){
        static bool DamageSent=false;
        if(!DamageSent&&W->GetTimeSeconds()>18)for(auto It=W->GetPlayerControllerIterator();It;++It)if(auto PC=It->Get())if(PC->GetNetConnection()&&PC->GetPawn()){Call(PC->GetPawn(),TEXT("GraphReceiveHit"));DamageSent=true;break;}
        static int Snapshot=-1;int Seconds=static_cast<int>(W->GetTimeSeconds());
        if(Seconds!=Snapshot){Snapshot=Seconds;FString D=FString::Printf(TEXT("World time: %d\n"),Seconds);for(auto A:Actors(W,CharacterClass))D+=FString::Printf(TEXT("%s hunter=%d bot=%d lives=%d replicates=%d location=%s\n"),*A->GetName(),Bool(A,TEXT("bHunter")),Bool(A,TEXT("bBot")),Number(A,TEXT("Lives")),A->GetIsReplicated(),*A->GetActorLocation().ToString());FFileHelper::SaveStringToFile(D,*(FPaths::ProjectSavedDir()/TEXT("PureBlueprintServerRoster.txt")));}
        if(W->GetTimeSeconds()>55)FPlatformMisc::RequestExit(false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintClient"))&&W->GetNetMode()==NM_Client){
        static int Stage=0;static float Before=0,Sent=0;auto S=W->GetGameState();auto PC=W->GetFirstPlayerController();
        if(Stage==0&&W->GetTimeSeconds()>12&&S&&PC&&Bool(S,TEXT("bStarted"))){Check(PC->GetPawn()&&Number(PC->GetPawn(),TEXT("Lives"))==2,TEXT("Server damage is replicated to the owning client"));Before=Real(S,TEXT("Remaining"));Sent=W->GetTimeSeconds();Stage=1;Call(PC,TEXT("ServerRestartRound"));}
        if(Stage==1&&W->GetTimeSeconds()-Sent>3){Stage=2;int Hunters=0;auto R=Actors(W,CharacterClass);for(auto A:R){if(Bool(A,TEXT("bHunter")))++Hunters;Report+=FString::Printf(TEXT("Actor %s hunter=%d lives=%d position=%s\n"),*A->GetName(),Bool(A,TEXT("bHunter")),Number(A,TEXT("Lives")),*A->GetActorLocation().ToString());}Report+=FString::Printf(TEXT("Roster=%d hunters=%d timer_before=%.1f after=%.1f\n"),R.Num(),Hunters,Before,Real(S,TEXT("Remaining")));Check(R.Num()==4&&Hunters==1,TEXT("Replicated four-character roster and one hunter"));Check(PC->GetPawn()&&Number(PC->GetPawn(),TEXT("Lives"))==3,TEXT("Client receives replicated lives"));Check(Real(S,TEXT("Remaining"))>Before,TEXT("Blueprint server RPC restarts round from client"));Finish(TEXT("PureBlueprintNetworkTest.txt"));}
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintPreview"))){
        static int Stage=0;float T=W->GetTimeSeconds();
        if(Stage==0&&T>10){Stage=1;Freeze(W);if(auto PC=W->GetFirstPlayerController())if(auto C=PC->GetPawn()){bool Assigned=false;for(auto A:Actors(W,CharacterClass)){SetBool(A,TEXT("bHunter"),A!=C&&!Assigned);if(A!=C)Assigned=true;}SetBool(W->GetGameState(),TEXT("bFinished"),false);SetInt(C,TEXT("Lives"),2);C->SetActorLocationAndRotation(FVector(50,-2350,100),FRotator(0,90,0));PC->SetControlRotation(FRotator(-7,90,0));}}
        if(Stage==1&&T>13){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("BlueprintHUD_Runner.png"),true,false);Stage=2;}
        if(Stage==2&&T>16){Stage=3;if(auto PC=W->GetFirstPlayerController())if(auto C=PC->GetPawn()){for(auto A:Actors(W,CharacterClass))SetBool(A,TEXT("bHunter"),A==C);SetBool(C,TEXT("bStick"),true);SetInt(C,TEXT("Lives"),3);}}
        if(Stage==3&&T>19){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("BlueprintHUD_Hunter.png"),true,false);Stage=4;}
        if(Stage==4&&T>21)FPlatformMisc::RequestExit(false);
    }
}
}
void StartBlueprintTestHarness(){if(FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintSelfTest"))||FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintServer"))||FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintClient"))||FParse::Param(FCommandLine::Get(),TEXT("PogonyaBlueprintPreview")))BlueprintTestTickHandle=FWorldDelegates::OnWorldPostActorTick.AddStatic(Tick);}
void StopBlueprintTestHarness(){if(BlueprintTestTickHandle.IsValid())FWorldDelegates::OnWorldPostActorTick.Remove(BlueprintTestTickHandle);}
