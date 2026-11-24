#include "PogonyaGraphBuilder.h"
#include "PureBlueprintTools.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "AIController.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace PureGame {
int Errors=0;
UBlueprint *Character=nullptr,*State=nullptr,*Pickup=nullptr,*Mode=nullptr,*Controller=nullptr,*HUD=nullptr,*Gate=nullptr;
static const TCHAR* Root=TEXT("/Game/Pogonya/Blueprints/");
static UBlueprint* NewBP(const TCHAR* Name,UClass* Parent){
    FString Path=FString(Root)+Name;
    if(FPackageName::DoesPackageExist(Path)){UE_LOG(LogTemp,Error,TEXT("PURE preserve existing %s; use the explicit rebuild script with its backup check"),*Path);++Errors;return nullptr;}
    auto BP=FKismetEditorUtilities::CreateBlueprint(Parent,CreatePackage(*Path),Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
    BP->bForceFullEditor=true;
    FKismetEditorUtilities::CompileBlueprint(BP);return BP;
}
static void Var(UBlueprint* BP,const TCHAR* N,const FEdGraphPinType& T,const TCHAR* D,const TCHAR* Category,bool Rep=false){
    if(!FBlueprintEditorUtils::AddMemberVariable(BP,N,T,D)){++Errors;return;}
    auto& V=BP->NewVariables.Last();V.PropertyFlags|=CPF_Edit|CPF_BlueprintVisible;
    if(Rep)V.PropertyFlags|=CPF_Net;
    V.Category=FText::FromString(Category);
    V.MetaDataArray.Add(FBPVariableMetaDataEntry(TEXT("Tooltip"),FString(Category)+TEXT(" / ")+N));
}
static void Bool(UBlueprint* B,const TCHAR* N,const TCHAR* D=TEXT("false"),bool Rep=true){Var(B,N,Type(UEdGraphSchema_K2::PC_Boolean),D,TEXT("Состояние игры"),Rep);}
static void Float(UBlueprint* B,const TCHAR* N,const TCHAR* D,bool Rep=false){Var(B,N,Type(UEdGraphSchema_K2::PC_Real),D,TEXT("Настройки"),Rep);}
static void Int(UBlueprint* B,const TCHAR* N,const TCHAR* D,bool Rep=false){Var(B,N,Type(UEdGraphSchema_K2::PC_Int),D,TEXT("Состояние игры"),Rep);}
static void Vector(UBlueprint* B,const TCHAR* N,const TCHAR* D){Var(B,N,Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get()),D,TEXT("Расположение"));}
static void Seed(UBlueprint* B,const TCHAR* Page,const TCHAR* Event,const TCHAR* Param=nullptr,bool Boolean=false,bool Server=false){
    FPage P(B,Page);auto E=P.Event(Event);
    if(Param)E->CreateUserDefinedPin(Param,Type(Boolean?UEdGraphSchema_K2::PC_Boolean:UEdGraphSchema_K2::PC_Real),EGPD_Output);
    if(Server)E->FunctionFlags|=FUNC_Net|FUNC_NetServer|FUNC_NetReliable;
}
static USCS_Node* Component(UBlueprint* B,UClass* T,const TCHAR* N,USCS_Node* Parent=nullptr){
    auto Node=B->SimpleConstructionScript->CreateNode(T,N);
    if(Parent)Parent->AddChildNode(Node);else B->SimpleConstructionScript->AddNode(Node);
    return Node;
}
static USCS_Node* Mesh(UBlueprint* B,const TCHAR* N,USCS_Node* Parent,const TCHAR* Shape,FVector Pos,FVector Scale,const TCHAR* Mat,bool Collision=false){
    auto Node=Component(B,UStaticMeshComponent::StaticClass(),N,Parent);auto M=CastChecked<UStaticMeshComponent>(Node->ComponentTemplate);
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(FString(TEXT("/Engine/BasicShapes/"))+Shape+TEXT(".")+Shape)));
    M->SetRelativeLocation(Pos);M->SetRelativeScale3D(Scale);
    M->SetCollisionProfileName(Collision?TEXT("BlockAllDynamic"):TEXT("NoCollision"));
    M->SetCanEverAffectNavigation(false);
    if(Mat)M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Pogonya/Materials/M_"))+Mat)));
    return Node;
}
static void Models(){
    auto C=Cast<ACharacter>(Character->GeneratedClass->GetDefaultObject());
    auto Figure=Component(Character,USceneComponent::StaticClass(),TEXT("Figure"));Figure->SetParent(C->GetCapsuleComponent());
    Cast<USceneComponent>(Figure->ComponentTemplate)->SetRelativeLocation(FVector(0,0,-88));
    auto Arm=Component(Character,USpringArmComponent::StaticClass(),TEXT("CameraBoom"));Arm->SetParent(C->GetCapsuleComponent());
    auto A=Cast<USpringArmComponent>(Arm->ComponentTemplate);A->TargetArmLength=320;A->SocketOffset=FVector(0,55,65);A->bUsePawnControlRotation=true;
    auto Camera=Component(Character,UCameraComponent::StaticClass(),TEXT("FollowCamera"),Arm);Camera->AttachToName=USpringArmComponent::SocketName;Cast<UCameraComponent>(Camera->ComponentTemplate)->FieldOfView=85;
    Mesh(Character,TEXT("Body"),Figure,TEXT("Cube"),FVector(0,0,113),FVector(.38,.55,.66),TEXT("Runner"));
    Mesh(Character,TEXT("Hood"),Figure,TEXT("Sphere"),FVector(0,0,159),FVector(.4,.42,.46),TEXT("Rubber"));
    Mesh(Character,TEXT("Mask"),Figure,TEXT("Sphere"),FVector(17,0,160),FVector(.1,.29,.27),TEXT("Mask"));
    Mesh(Character,TEXT("Visor"),Figure,TEXT("Cube"),FVector(22,0,165),FVector(.025,.26,.055),TEXT("Rubber"));
    Mesh(Character,TEXT("LeftLeg"),Figure,TEXT("Cube"),FVector(0,-15,42),FVector(.2,.22,.8),TEXT("Rubber"));
    Mesh(Character,TEXT("RightLeg"),Figure,TEXT("Cube"),FVector(0,15,42),FVector(.2,.22,.8),TEXT("Rubber"));
    Mesh(Character,TEXT("LeftArm"),Figure,TEXT("Cube"),FVector(0,-36,111),FVector(.2,.18,.65),TEXT("Runner"));
    Mesh(Character,TEXT("RightArm"),Figure,TEXT("Cube"),FVector(0,36,111),FVector(.2,.18,.65),TEXT("Runner"));
    Mesh(Character,TEXT("Backpack"),Figure,TEXT("Cube"),FVector(-26,0,116),FVector(.18,.34,.43),TEXT("Rubber"));
    Mesh(Character,TEXT("Weapon"),Figure,TEXT("Cube"),FVector(30,40,85),FVector(.07,.07,1.05),TEXT("Wood"));
    auto R=Component(Pickup,USceneComponent::StaticClass(),TEXT("ItemRoot"));
    Mesh(Pickup,TEXT("StickMesh"),R,TEXT("Cube"),FVector::ZeroVector,FVector(.08,.08,1.05),TEXT("Wood"));
    Mesh(Pickup,TEXT("KeyRing"),R,TEXT("Cylinder"),FVector::ZeroVector,FVector(.18,.18,.05),TEXT("Key"));
    Mesh(Pickup,TEXT("KeyShaft"),R,TEXT("Cube"),FVector(15,0,0),FVector(.28,.045,.045),TEXT("Key"));
    Mesh(Pickup,TEXT("KeyTooth"),R,TEXT("Cube"),FVector(27,5,0),FVector(.05,.13,.045),TEXT("Key"));
    auto Glow=Cast<UPointLightComponent>(Component(Pickup,UPointLightComponent::StaticClass(),TEXT("ItemGlow"),R)->ComponentTemplate);
    Glow->SetIntensity(200);Glow->SetAttenuationRadius(140);Glow->SetLightColor(FLinearColor(1,.6,.15));Glow->SetCastShadows(false);
    auto G=Component(Gate,USceneComponent::StaticClass(),TEXT("GateRoot"));
    Mesh(Gate,TEXT("LeftDoor"),G,TEXT("Cube"),FVector(-300,0,255),FVector(5.8,.16,4.8),TEXT("Rust"),true);
    Mesh(Gate,TEXT("RightDoor"),G,TEXT("Cube"),FVector(300,0,255),FVector(5.8,.16,4.8),TEXT("Rust"),true);
}
static void Inputs(){
    FPage P(Character,TEXT("00_Input_and_Network"));
    const TCHAR* Axes[]={TEXT("MoveForward"),TEXT("MoveRight"),TEXT("Turn"),TEXT("LookUp")};
    const TCHAR* Events[]={TEXT("GraphMoveForward"),TEXT("GraphMoveRight"),TEXT("GraphLookYaw"),TEXT("GraphLookPitch")};
    for(int i=0;i<4;i++){
        auto E=P.Node<UK2Node_InputAxisEvent>(0,i*350);E->Initialize(Axes[i]);E->AllocateDefaultPins();
        auto N=P.Call(CC(),Events[i],450,i*350);Wire(Then(E),Pin(N,TEXT("execute")));Wire(Pin(E,TEXT("AxisValue")),Pin(N,TEXT("Value")));
    }
    const TCHAR* Actions[]={TEXT("Interact"),TEXT("Attack"),TEXT("Jump"),TEXT("Sprint")};
    for(int i=0;i<4;i++){
        int Y=1700+i*650;auto E=P.Node<UK2Node_InputAction>(0,Y);E->InputActionName=Actions[i];E->AllocateDefaultPins();
        if(i<2){auto N=P.Call(CC(),i?TEXT("GraphAttack"):TEXT("GraphInteract"),500,Y);Wire(Pin(E,TEXT("Pressed")),Pin(N,TEXT("execute")));}
        else for(int release=0;release<2;release++){
            auto N=P.Call(CC(),i==2?TEXT("GraphJump"):TEXT("GraphSprint"),500,Y+release*270);Wire(Pin(E,release?TEXT("Released"):TEXT("Pressed")),Pin(N,TEXT("execute")));Value(Pin(N,i==2?TEXT("Pressed"):TEXT("Enabled")),release?TEXT("false"):TEXT("true"));
            if(i==3){auto RPC=P.Call(CC(),TEXT("ServerSprint"),850,Y+release*270);Wire(Then(N),Pin(RPC,TEXT("execute")));Value(Pin(RPC,TEXT("Enabled")),release?TEXT("false"):TEXT("true"));}
        }
    }
    P.Comment(TEXT("УПРАВЛЕНИЕ — события клавиш вызывают рабочие графы. Подбор и удар исполняются на сервере."),-70,-110,1250,4500);
    FPage S(Character,TEXT("02_Camera_and_Sprint"));auto E=S.Event(TEXT("ServerSprint"),2300);auto N=S.Call(CC(),TEXT("GraphSprint"),500,2300);Wire(Then(E),Pin(N,TEXT("execute")));Wire(Pin(E,TEXT("Enabled")),Pin(N,TEXT("Enabled")));
    FPage Q(Controller,TEXT("01_Restart_Request"));auto I=Q.Node<UK2Node_InputAction>(0,0);I->InputActionName=TEXT("Restart");I->AllocateDefaultPins();Q.Do(Pin(I,TEXT("Pressed")),Controller->GeneratedClass,TEXT("ServerRestartRound"),400,0);
    auto R=Q.Event(TEXT("ServerRestartRound"),500);auto GM=Q.Cast(Then(R),Out(Q.Call(UGameplayStatics::StaticClass(),TEXT("GetGameMode"),0,800)),MC(),400,500);Q.Do(Then(GM),MC(),TEXT("StartRound"),800,500,GM->GetCastResultPin());
    Q.Comment(TEXT("R — запрос нового раунда передаётся серверу через надёжное сетевое событие."),-60,-100,1250,1150);
}

bool BuildAll(){
    Errors=0;
    State=NewBP(TEXT("BP_PogonyaGameState"),AGameStateBase::StaticClass());
    Pickup=NewBP(TEXT("BP_PogonyaPickup"),AActor::StaticClass());
    Character=NewBP(TEXT("BP_PogonyaCharacter"),ACharacter::StaticClass());
    Controller=NewBP(TEXT("BP_PogonyaController"),APlayerController::StaticClass());
    HUD=NewBP(TEXT("BP_PogonyaHUD"),AHUD::StaticClass());
    Gate=NewBP(TEXT("BP_PogonyaGate"),AActor::StaticClass());
    Mode=NewBP(TEXT("BP_PogonyaGameMode"),AGameModeBase::StaticClass());
    if(Errors)return false;
    Bool(State,TEXT("bStarted"));Bool(State,TEXT("bFinished"));Bool(State,TEXT("bGateOpen"));
    Float(State,TEXT("Remaining"),TEXT("8"),true);Int(State,TEXT("Escaped"),TEXT("0"),true);
    Var(State,TEXT("Result"),Type(UEdGraphSchema_K2::PC_String),TEXT(""),TEXT("Раунд"),true);
    Bool(Pickup,TEXT("bKey"));Seed(Pickup,TEXT("01_Type_and_Appearance"),TEXT("UpdateAppearance"));
    for(auto N:{TEXT("bHunter"),TEXT("bStick"),TEXT("bKey"),TEXT("bOut"),TEXT("bEscaped")})Bool(Character,N);
    Bool(Character,TEXT("bBot"),TEXT("false"),false);Bool(Character,TEXT("BotFoundTarget"),TEXT("false"),false);
    Int(Character,TEXT("Lives"),TEXT("3"),true);
    Float(Character,TEXT("LastSwing"),TEXT("-10"),true);Float(Character,TEXT("LastHit"),TEXT("-10"),true);
    for(auto Pair:TArray<TPair<const TCHAR*,const TCHAR*>>{{TEXT("WalkSpeed"),TEXT("420")},{TEXT("RunnerSprintSpeed"),TEXT("570")},{TEXT("HunterSprintSpeed"),TEXT("610")},{TEXT("PickupRange"),TEXT("190")},{TEXT("AttackRange"),TEXT("180")},{TEXT("AttackCooldown"),TEXT("0.8")},{TEXT("HitInvulnerability"),TEXT("1.2")},{TEXT("ExitRange"),TEXT("260")},{TEXT("NextDecision"),TEXT("0")},{TEXT("BestDistance"),TEXT("999999")},{TEXT("BestSteerScore"),TEXT("-999999")}})Float(Character,Pair.Key,Pair.Value);
    Vector(Character,TEXT("ExitLocation"),TEXT("(X=0,Y=2700,Z=100)"));Vector(Character,TEXT("BotTarget"),TEXT("(X=0,Y=-2000,Z=100)"));Vector(Character,TEXT("BotSteer"),TEXT("(X=0,Y=1,Z=0)"));
    Var(Character,TEXT("BotThreat"),Type(UEdGraphSchema_K2::PC_Object,AActor::StaticClass()),TEXT("None"),TEXT("Боты"));
    const TCHAR* ChPages[]={TEXT("01_Movement"),TEXT("01_Movement"),TEXT("02_Camera_and_Sprint"),TEXT("02_Camera_and_Sprint")};
    const TCHAR* ChEvents[]={TEXT("GraphMoveForward"),TEXT("GraphMoveRight"),TEXT("GraphLookYaw"),TEXT("GraphLookPitch")};
    for(int i=0;i<4;i++)Seed(Character,ChPages[i],ChEvents[i],TEXT("Value"));
    Seed(Character,TEXT("02_Camera_and_Sprint"),TEXT("GraphSprint"),TEXT("Enabled"),true);
    Seed(Character,TEXT("02_Camera_and_Sprint"),TEXT("ServerSprint"),TEXT("Enabled"),true,true);
    Seed(Character,TEXT("06_Jump"),TEXT("GraphJump"),TEXT("Pressed"),true);
    Seed(Character,TEXT("03_Pickups_and_Escape"),TEXT("GraphInteract"),nullptr,false,true);
    Seed(Character,TEXT("04_Stick_Attack"),TEXT("GraphAttack"),nullptr,false,true);
    Seed(Character,TEXT("05_Three_Lives_and_Death"),TEXT("GraphReceiveHit"));
    Seed(Character,TEXT("07_Appearance"),TEXT("RefreshLook"));
    Seed(Character,TEXT("09_Bot_Decision"),TEXT("BotDecide"));Seed(Character,TEXT("10_Bot_Steering"),TEXT("BotMove"));
    Seed(Controller,TEXT("01_Restart_Request"),TEXT("ServerRestartRound"),nullptr,false,true);
    Float(Mode,TEXT("LobbyTime"),TEXT("8"));Float(Mode,TEXT("RoundDuration"),TEXT("360"));
    for(auto N:{TEXT("HumanCount"),TEXT("RosterCount"),TEXT("HunterIndex"),TEXT("KeyIndex"),TEXT("StickIndex"),TEXT("SpawnIndex"),TEXT("ActiveRunners"),TEXT("ActiveHunters")})Int(Mode,N,TEXT("0"));
    Vector(Mode,TEXT("KeyPosition"),TEXT("(X=-1770,Y=-1850,Z=80)"));Vector(Mode,TEXT("StickPosition"),TEXT("(X=1740,Y=1770,Z=80)"));
    Seed(Mode,TEXT("02_Start_Round"),TEXT("StartRound"));Seed(Mode,TEXT("03_Resolve_Round"),TEXT("ResolveRound"));
    Float(Gate,TEXT("OpenAmount"),TEXT("0"));
    Models();
    const TArray<UBlueprint*> All={State,Pickup,Character,Controller,HUD,Gate,Mode};
    for(auto B:All){FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(B);FKismetEditorUtilities::CompileBlueprint(B);}
    Inputs();BuildCharacterLogic();BuildCharacterVisuals();BuildBotLogic();BuildRoundLogic();BuildHUDLogic();BuildPickupGateLogic();
    for(auto B:All)LayoutBlueprint(B);
    FString Audit=TEXT("PURE BLUEPRINT GAME — engine base classes; no project-native gameplay calls\n");
    for(auto B:All){FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(B);FCompilerResultsLog L;FKismetEditorUtilities::CompileBlueprint(B,EBlueprintCompileOptions::None,&L);Errors+=L.NumErrors;Audit+=FString::Printf(TEXT("%s: parent=%s errors=%d warnings=%d\n"),*B->GetName(),*B->ParentClass->GetPathName(),L.NumErrors,L.NumWarnings);for(auto G:B->UbergraphPages)Audit+=FString::Printf(TEXT("  %s: %d nodes\n"),*G->GetName(),G->Nodes.Num());}
    // Set native engine component defaults after final compilation.
    auto C=Cast<ACharacter>(CC()->GetDefaultObject());C->SetReplicates(true);C->SetReplicateMovement(true);C->PrimaryActorTick.bCanEverTick=true;C->bUseControllerRotationYaw=false;C->AIControllerClass=AAIController::StaticClass();
    C->GetCapsuleComponent()->InitCapsuleSize(34,88);auto Move=C->GetCharacterMovement();Move->bOrientRotationToMovement=true;Move->RotationRate=FRotator(0,620,0);Move->MaxWalkSpeed=420;Move->JumpZVelocity=460;Move->AirControl=.3;
    for(auto B:{Pickup,Gate}){auto A=Cast<AActor>(B->GeneratedClass->GetDefaultObject());A->SetReplicates(true);A->SetReplicateMovement(true);A->PrimaryActorTick.bCanEverTick=true;}
    auto GM=Cast<AGameModeBase>(MC()->GetDefaultObject());GM->DefaultPawnClass=CC();GM->GameStateClass=SC();GM->PlayerControllerClass=Controller->GeneratedClass;GM->HUDClass=HUD->GeneratedClass;GM->PrimaryActorTick.bCanEverTick=true;
    for(auto B:All){for(auto G:B->UbergraphPages)for(auto N:G->Nodes)if(auto Call=Cast<UK2Node_CallFunction>(N)){auto F=Call->GetTargetFunction();if(F&&F->GetOutermost()->GetName().StartsWith(TEXT("/Script/Pogonya"))){++Errors;Audit+=TEXT("FORBIDDEN native gameplay reference\n");}}}
    Audit+=FString::Printf(TEXT("TOTAL ERRORS: %d\n"),Errors);FFileHelper::SaveStringToFile(Audit,*(FPaths::ProjectSavedDir()/TEXT("PureBlueprintAudit.txt")));
    if(Errors){UE_LOG(LogTemp,Error,TEXT("%s"),*Audit);return false;}
    for(auto B:All){B->BlueprintDescription=TEXT("Игровая логика реализована стандартными узлами Blueprint. Переменные доступны в My Blueprint, параметры — Class Defaults.");B->LastEditedDocuments.Reset();if(B->UbergraphPages.Num()>1)B->LastEditedDocuments.Add(FEditedDocumentInfo(B->UbergraphPages[1],FVector2D(-120,-160),.65));FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;FString File=FPackageName::LongPackageNameToFilename(B->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());if(!UPackage::SavePackage(B->GetOutermost(),B,*File,Args))++Errors;}
    UE_LOG(LogTemp,Display,TEXT("%s"),*Audit);return Errors==0;
}
}
bool UPogonyaGraphBuilder::BuildBlueprintGame(){return PureGame::BuildAll();}
bool UPogonyaGraphBuilder::OpenBlueprintGameGraphs(){
    const TPair<const TCHAR*,const TCHAR*> Pages[]={
        {TEXT("BP_PogonyaHUD"),TEXT("01_Draw_Interface")},
        {TEXT("BP_PogonyaGameMode"),TEXT("02_Start_Round")},
        {TEXT("BP_PogonyaCharacter"),TEXT("05_Three_Lives_and_Death")}
    };
    for(auto Item:Pages){
        auto B=LoadObject<UBlueprint>(nullptr,*(FString(TEXT("/Game/Pogonya/Blueprints/"))+Item.Key));if(!B)return false;
        UEdGraph* Page=nullptr;for(auto G:B->UbergraphPages)if(G->GetFName()==FName(Item.Value)){Page=G;break;}if(!Page)return false;
        FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Page);
    }
    return true;
}
