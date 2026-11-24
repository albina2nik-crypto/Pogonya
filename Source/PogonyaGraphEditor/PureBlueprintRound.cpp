#include "PureBlueprintTools.h"
#include "Kismet/KismetArrayLibrary.h"

namespace PureGame {
static UEdGraphPin* Increment(FPage& P,UEdGraphPin* E,const TCHAR* Name,int X,int Y){return P.AssignPin(E,Name,P.Op(TEXT("Add_IntInt"),P.Get(Name,X-250,Y+200),nullptr,X,Y+200,TEXT("1")),X+250,Y);}
static UEdGraphPin* Spot(FPage& P,UEdGraphPin* Index,int X,int Y){
    auto N=P.Node<UK2Node_Select>(X,Y);N->AllocateDefaultPins();for(int i=0;i<6;i++)N->AddInputPin();auto R=Out(N);R->PinType=Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get());N->PinTypeChanged(R);Wire(Index,Pin(N,TEXT("Index")));
    const TCHAR* Spots[]={TEXT("-1770,-1850,80"),TEXT("1740,1770,80"),TEXT("-1770,1330,80"),TEXT("1750,-950,80"),TEXT("200,1480,80"),TEXT("-100,-1400,80"),TEXT("1170,200,80"),TEXT("-1150,400,80")};
    TArray<UEdGraphPin*> Opt;N->GetOptionPins(Opt);for(int i=0;i<Opt.Num();i++)Value(Opt[i],Spots[i]);return R;
}
static void Start(){
    FPage P(Mode,TEXT("02_Start_Round"));auto E=P.Event(TEXT("StartRound"));auto S=P.State(0,350);auto Valid=P.Branch(P.Authority(Then(E),300,0),P.Valid(S,600,350),900,0);
    auto Items=P.Loop(Then(Valid),PC(),false,1500,0);P.Do(Pin(Items,TEXT("LoopBody")),AActor::StaticClass(),TEXT("K2_DestroyActor"),1900,0,Pin(Items,TEXT("Array Element")));
    auto Exec=P.Assign(Pin(Items,TEXT("Completed")),TEXT("HumanCount"),TEXT("0"),1900,450);
    auto Existing=P.Loop(Exec,CC(),false,2500,450);auto C=P.Cast(Pin(Existing,TEXT("LoopBody")),Pin(Existing,TEXT("Array Element")),CC(),2800,450);auto Ch=C->GetCastResultPin();
    auto Bot=P.Branch(Then(C),P.Get(TEXT("bBot"),2800,700,CC(),Ch),3100,450);
    P.Do(Then(Bot),AActor::StaticClass(),TEXT("K2_DestroyActor"),3400,450,Ch);Increment(P,Pin(Bot,TEXT("else")),TEXT("HumanCount"),3400,950);
    auto Fill=P.Macro(TEXT("ForLoop"),2900,1450);Wire(Pin(Existing,TEXT("Completed")),Pin(Fill,TEXT("execute")));Wire(P.Get(TEXT("HumanCount"),2600,1700),Pin(Fill,TEXT("FirstIndex")));Value(Pin(Fill,TEXT("LastIndex")),TEXT("3"));
    auto Spawn=P.Spawn(Pin(Fill,TEXT("LoopBody")),CC(),P.Literal(Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get()),TEXT("0,-2250,100"),3000,1950),3400,1450);
    Exec=P.Assign(Then(Spawn),TEXT("bBot"),TEXT("true"),3750,1450,CC(),Out(Spawn));P.Do(Exec,APawn::StaticClass(),TEXT("SpawnDefaultController"),4100,1450,Out(Spawn));
    auto All=P.Call(UGameplayStatics::StaticClass(),TEXT("GetAllActorsOfClass"),3400,2350);Pin(All,TEXT("ActorClass"))->DefaultObject=CC();Wire(Pin(Fill,TEXT("Completed")),Pin(All,TEXT("execute")));
    auto Len=P.Call(UKismetArrayLibrary::StaticClass(),TEXT("Array_Length"),3700,2650);Wire(Pin(All,TEXT("OutActors")),Pin(Len,TEXT("TargetArray")));
    Exec=P.AssignPin(Then(All),TEXT("RosterCount"),Out(Len),4050,2350);
    auto Max=P.Op(TEXT("Subtract_IntInt"),P.Get(TEXT("RosterCount"),4050,2700),nullptr,4300,2700,TEXT("1"));
    auto Rand=P.Math(TEXT("RandomIntegerInRange"),4550,2700);Wire(Max,Pin(Rand,TEXT("Max")));
    Exec=P.AssignPin(Exec,TEXT("HunterIndex"),Out(Rand),4550,2350);
    auto Reset=P.Macro(TEXT("ForEachLoop"),4900,2350);Wire(Exec,Pin(Reset,TEXT("Exec")));Wire(Pin(All,TEXT("OutActors")),Pin(Reset,TEXT("Array")));
    auto Cast=P.Cast(Pin(Reset,TEXT("LoopBody")),Pin(Reset,TEXT("Array Element")),CC(),5200,2350);Ch=Cast->GetCastResultPin();
    auto Role=P.Op(TEXT("EqualEqual_IntInt"),Pin(Reset,TEXT("Array Index")),P.Get(TEXT("HunterIndex"),5200,2700),5500,2700);
    Exec=P.AssignPin(Then(Cast),TEXT("bHunter"),Role,5800,2350,CC(),Ch);
    int X=6100;for(auto V:{TEXT("bStick"),TEXT("bKey"),TEXT("bOut"),TEXT("bEscaped")}){Exec=P.Assign(Exec,V,TEXT("false"),X,2350,CC(),Ch);X+=280;}
    Exec=P.Assign(Exec,TEXT("Lives"),TEXT("3"),X,2350,CC(),Ch);X+=280;
    Exec=P.Assign(Exec,TEXT("LastHit"),TEXT("-10"),X,2350,CC(),Ch);X+=280;Exec=P.Assign(Exec,TEXT("LastSwing"),TEXT("-10"),X,2350,CC(),Ch);X+=280;
    auto Move=P.Get(TEXT("CharacterMovement"),X,2700,CC(),Ch);Exec=P.Do(Exec,UMovementComponent::StaticClass(),TEXT("StopMovementImmediately"),X,2350,Move);X+=330;
    Exec=P.AssignPin(Exec,TEXT("MaxWalkSpeed"),P.Get(TEXT("WalkSpeed"),X,2900,CC(),Ch),X,2350,UCharacterMovementComponent::StaticClass(),Move);X+=330;
    auto Walk=P.Call(UCharacterMovementComponent::StaticClass(),TEXT("SetMovementMode"),X,2350,Move);Wire(Exec,Pin(Walk,TEXT("execute")));Value(Pin(Walk,TEXT("NewMovementMode")),TEXT("MOVE_Walking"));X+=350;
    auto IndexFloat=P.Math(TEXT("Conv_IntToDouble"),5500,3150);Wire(Pin(Reset,TEXT("Array Index")),Pin(IndexFloat,TEXT("InInt")));
    auto PosX=P.Op(TEXT("Add_DoubleDouble"),P.Op(TEXT("Multiply_DoubleDouble"),Out(IndexFloat),nullptr,5800,3150,TEXT("350")),nullptr,6100,3150,TEXT("-600"));
    auto Pos=P.Math(TEXT("MakeVector"),6400,3150);Wire(PosX,Pin(Pos,TEXT("X")));Value(Pin(Pos,TEXT("Y")),TEXT("-2250"));Value(Pin(Pos,TEXT("Z")),TEXT("100"));
    auto Tele=P.Call(AActor::StaticClass(),TEXT("K2_SetActorLocationAndRotation"),X,2350,Ch);Wire(Then(Walk),Pin(Tele,TEXT("execute")));Wire(Out(Pos),Pin(Tele,TEXT("NewLocation")));Value(Pin(Tele,TEXT("NewRotation")),TEXT("0,90,0"));Value(Pin(Tele,TEXT("bTeleport")),TEXT("true"));X+=450;
    auto Con=P.Call(APawn::StaticClass(),TEXT("GetController"),X,2750,Ch);auto Rot=P.Call(AController::StaticClass(),TEXT("SetControlRotation"),X,2350,Out(Con));Wire(Then(Tele),Pin(Rot,TEXT("execute")));auto Facing=P.Math(TEXT("MakeRotator"),X,3050);Value(Pin(Facing,TEXT("Pitch")),TEXT("-10"));Value(Pin(Facing,TEXT("Yaw")),TEXT("90"));Wire(Out(Facing),Pin(Rot,TEXT("NewRotation")));
    P.Do(Then(Rot),CC(),TEXT("RefreshLook"),X+350,2350,Ch);
    auto KeyRand=P.Math(TEXT("RandomIntegerInRange"),5200,4000);Value(Pin(KeyRand,TEXT("Max")),TEXT("7"));Exec=P.AssignPin(Pin(Reset,TEXT("Completed")),TEXT("KeyIndex"),Out(KeyRand),5500,3750);
    auto Offset=P.Math(TEXT("RandomIntegerInRange"),5500,4200);Value(Pin(Offset,TEXT("Min")),TEXT("1"));Value(Pin(Offset,TEXT("Max")),TEXT("7"));
    auto Index=P.Op(TEXT("Percent_IntInt"),P.Op(TEXT("Add_IntInt"),P.Get(TEXT("KeyIndex"),5800,4100),Out(Offset),6100,4100),nullptr,6400,4100,TEXT("8"));
    Exec=P.AssignPin(Exec,TEXT("StickIndex"),Index,6400,3750);
    auto Key=P.Spawn(Exec,PC(),Spot(P,P.Get(TEXT("KeyIndex"),6400,4550),6800,4450),7100,3750);
    Exec=P.Assign(Then(Key),TEXT("bKey"),TEXT("true"),7500,3750,PC(),Out(Key));Exec=P.Do(Exec,PC(),TEXT("UpdateAppearance"),7800,3750,Out(Key));
    auto Stick=P.Spawn(Exec,PC(),Spot(P,P.Get(TEXT("StickIndex"),7800,4550),8100,4450),8500,3750);
    Exec=P.Assign(Then(Stick),TEXT("bKey"),TEXT("false"),8900,3750,PC(),Out(Stick));Exec=P.Do(Exec,PC(),TEXT("UpdateAppearance"),9200,3750,Out(Stick));
    Exec=P.Assign(Exec,TEXT("bStarted"),TEXT("true"),9550,3750,SC(),S);Exec=P.Assign(Exec,TEXT("bFinished"),TEXT("false"),9850,3750,SC(),S);Exec=P.Assign(Exec,TEXT("bGateOpen"),TEXT("false"),10150,3750,SC(),S);
    Exec=P.Assign(Exec,TEXT("Escaped"),TEXT("0"),10450,3750,SC(),S);Exec=P.Assign(Exec,TEXT("Result"),TEXT(""),10750,3750,SC(),S);
    P.AssignPin(Exec,TEXT("Remaining"),P.Get(TEXT("RoundDuration"),10750,4100),11050,3750,SC(),S);
    P.Comment(TEXT("1 — Удалить прошлые предметы и ботов, сохранить настоящих игроков"),-100,-150,4000,1450);
    P.Comment(TEXT("2 — Дополнить состав до четырёх и выбрать ровно одного гонщика"),2450,1320,2750,2150);
    P.Comment(TEXT("3 — ТРИ ЖИЗНИ: Set Lives = 3; сброс предметов, выбывания и позиции"),5150,2200,6000,1250);
    P.Comment(TEXT("4 — Разные тайники для ключа и палки; 360 секунд и закрытый выход"),5150,3600,6500,1900);
}
static void Resolve(){
    FPage P(Mode,TEXT("03_Resolve_Round"));auto E=P.Event(TEXT("ResolveRound"));auto S=P.State(0,300);auto Exec=P.Assign(Then(E),TEXT("ActiveRunners"),TEXT("0"),350,0);Exec=P.Assign(Exec,TEXT("ActiveHunters"),TEXT("0"),650,0);
    auto L=P.Loop(Exec,CC(),false,1250,0);auto C=P.Cast(Pin(L,TEXT("LoopBody")),Pin(L,TEXT("Array Element")),CC(),1600,0);auto Ch=C->GetCastResultPin();
    auto Alive=P.Branch(Then(C),P.Not(P.Get(TEXT("bOut"),1600,350,CC(),Ch),1850,350),2100,0);
    auto Hunter=P.Branch(Then(Alive),P.Get(TEXT("bHunter"),2100,500,CC(),Ch),2450,0);Increment(P,Then(Hunter),TEXT("ActiveHunters"),2800,0);Increment(P,Pin(Hunter,TEXT("else")),TEXT("ActiveRunners"),2800,650);
    auto NoR=P.Compare(TEXT("EqualEqual_IntInt"),P.Get(TEXT("ActiveRunners"),1600,1350),nullptr,1900,1350,TEXT("0"));
    auto NoH=P.Compare(TEXT("EqualEqual_IntInt"),P.Get(TEXT("ActiveHunters"),1600,1550),nullptr,1900,1550,TEXT("0"));
    auto Timeout=P.Compare(TEXT("LessEqual_DoubleDouble"),P.Get(TEXT("Remaining"),1600,1800,SC(),S),nullptr,1900,1800,TEXT("0"));
    auto End=P.Branch(Pin(L,TEXT("Completed")),P.Or(P.Or(NoR,NoH,2200,1400),Timeout,2500,1450),2800,1150);
    Exec=P.Assign(Then(End),TEXT("bFinished"),TEXT("true"),3150,1150,SC(),S);
    auto Escape=P.Compare(TEXT("Greater_IntInt"),P.Get(TEXT("Escaped"),3150,1500,SC(),S),nullptr,3450,1500,TEXT("0"));
    auto Result=P.Choose(NoH,P.Str(TEXT("ГОНЩИК ПОКИНУЛ РАУНД"),3450,1900),P.Choose(Escape,P.Str(TEXT("БЕГЛЕЦЫ ПОБЕДИЛИ"),3450,2150),P.Str(TEXT("ГОНЩИК ПОБЕДИЛ"),3450,2350),3800,2100),4150,1800);
    P.AssignPin(Exec,TEXT("Result"),Result,4500,1150,SC(),S);
    P.Comment(TEXT("ИСХОД — нет активных беглецов, нет гонщика или истёк таймер. Хотя бы один спасшийся даёт победу беглецам."),-80,-140,5000,2800);
}
void BuildRoundLogic(){
    Start();Resolve();FPage P(Mode,TEXT("01_Lobby_and_Timer"));auto E=P.NativeEvent(AActor::StaticClass(),TEXT("ReceiveTick"));auto S=P.State(0,450);
    auto Ready=P.Branch(P.Authority(Then(E),300,0),P.Valid(S,550,450),850,0);auto Started=P.Branch(Then(Ready),P.Get(TEXT("bStarted"),850,400,SC(),S),1200,0);
    auto Countdown=P.Op(TEXT("Subtract_DoubleDouble"),P.Get(TEXT("LobbyTime"),1200,850),Pin(E,TEXT("DeltaSeconds")),1500,850);
    auto Exec=P.AssignPin(Pin(Started,TEXT("else")),TEXT("LobbyTime"),Countdown,1800,700);
    auto Clamp=P.Math(TEXT("FMax"),2100,1050);Wire(P.Get(TEXT("LobbyTime"),1800,1100),Pin(Clamp,TEXT("A")));Value(Pin(Clamp,TEXT("B")),TEXT("0"));Exec=P.AssignPin(Exec,TEXT("Remaining"),Out(Clamp),2400,700,SC(),S);
    auto Begin=P.Branch(Exec,P.Compare(TEXT("LessEqual_DoubleDouble"),P.Get(TEXT("LobbyTime"),2400,1050),nullptr,2700,1050,TEXT("0")),3000,700);P.Do(Then(Begin),MC(),TEXT("StartRound"),3350,700);
    auto Active=P.Branch(Then(Started),P.Not(P.Get(TEXT("bFinished"),1500,350,SC(),S),1800,350),2100,0);
    auto Minus=P.Op(TEXT("Subtract_DoubleDouble"),P.Get(TEXT("Remaining"),2100,1600,SC(),S),Pin(E,TEXT("DeltaSeconds")),2400,1600);auto Limit=P.Math(TEXT("FMax"),2700,1600);Wire(Minus,Pin(Limit,TEXT("A")));Value(Pin(Limit,TEXT("B")),TEXT("0"));
    Exec=P.AssignPin(Then(Active),TEXT("Remaining"),Out(Limit),3000,0,SC(),S);P.Do(Exec,MC(),TEXT("ResolveRound"),3350,0);
    P.Comment(TEXT("ТАЙМЕР — сначала ожидание 8 секунд, затем игровой раунд. Время меняет только сервер."),-80,-130,3900,2050);
    FPage J(Mode,TEXT("04_Join_and_Leave"));auto Join=J.NativeEvent(AGameModeBase::StaticClass(),TEXT("K2_PostLogin"));auto Delay=J.Call(UKismetSystemLibrary::StaticClass(),TEXT("Delay"),350,0);Wire(Then(Join),Pin(Delay,TEXT("execute")));Value(Pin(Delay,TEXT("Duration")),TEXT("0.3"));auto JS=J.State(0,300);auto V=J.Branch(Then(Delay),J.Valid(JS,550,300),850,0);auto Playing=J.Branch(Then(V),J.Get(TEXT("bStarted"),850,350,SC(),JS),1200,0);J.Do(Then(Playing),MC(),TEXT("StartRound"),1550,0);
    auto Leave=J.NativeEvent(AGameModeBase::StaticClass(),TEXT("K2_OnLogout"),850);auto Pawn=J.Call(AController::StaticClass(),TEXT("K2_GetPawn"),350,1150,Pin(Leave,TEXT("ExitingController")));auto C=J.Cast(Then(Leave),Out(Pawn),CC(),700,850);auto Ch=C->GetCastResultPin();
    Exec=J.Assign(Then(C),TEXT("bOut"),TEXT("true"),1050,850,CC(),Ch);auto Key=J.Branch(Exec,J.Get(TEXT("bKey"),1050,1250,CC(),Ch),1400,850);
    auto Drop=J.Spawn(Then(Key),PC(),J.Position(1400,1500,Ch),1800,850);Exec=J.Assign(Then(Drop),TEXT("bKey"),TEXT("true"),2150,850,PC(),Out(Drop));J.Do(Exec,PC(),TEXT("UpdateAppearance"),2500,850,Out(Drop));
    J.Comment(TEXT("ПОДКЛЮЧЕНИЕ — новый игрок включается в новый раунд. При выходе носителя ключ остаётся на карте."),-80,-130,3000,2050);
}
}
