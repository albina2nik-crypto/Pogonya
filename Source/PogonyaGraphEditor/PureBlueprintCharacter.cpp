#include "PureBlueprintTools.h"
namespace PureGame {
static void MovementGraphs(UBlueprint* BP){
    FPage P(BP,TEXT("01_Movement"));
    for(int Row=0;Row<2;Row++){
        int Y=Row*800;auto E=P.Event(Row?TEXT("GraphMoveRight"):TEXT("GraphMoveForward"),Y);
        auto Allowed=P.Not(P.Get(TEXT("bOut"),0,Y+220),220,Y+220);auto B=P.Branch(Pin(E,TEXT("then")),Allowed,450,Y);
        auto Rotation=P.Call(APawn::StaticClass(),TEXT("GetControlRotation"),450,Y+170);
        auto Break=P.Math(TEXT("BreakRotator"),700,Y+170);Wire(Pin(Rotation,TEXT("ReturnValue")),Pin(Break,TEXT("InRot")));
        auto Make=P.Math(TEXT("MakeRotator"),950,Y+170);Wire(Pin(Break,TEXT("Yaw")),Pin(Make,TEXT("Yaw")));
        auto Direction=P.Math(Row?TEXT("GetRightVector"):TEXT("GetForwardVector"),1200,Y+170);Wire(Pin(Make,TEXT("ReturnValue")),Pin(Direction,TEXT("InRot")));
        auto Move=P.Call(APawn::StaticClass(),TEXT("AddMovementInput"),1480,Y);Wire(Pin(B,TEXT("then")),Pin(Move,TEXT("execute")));Wire(Pin(Direction,TEXT("ReturnValue")),Pin(Move,TEXT("WorldDirection")));Wire(Pin(E,TEXT("Value")),Pin(Move,TEXT("ScaleValue")));
        P.Comment(Row?TEXT("MOVE RIGHT / LEFT — camera yaw, ignore eliminated characters"):TEXT("MOVE FORWARD / BACK — camera yaw, ignore eliminated characters"),-50,Y-100,1850,650);
    }
}
static void CameraSprintGraphs(UBlueprint* BP){
    FPage P(BP,TEXT("02_Camera_and_Sprint"));
    for(int Row=0;Row<2;Row++){
        int Y=Row*450;auto E=P.Event(Row?TEXT("GraphLookPitch"):TEXT("GraphLookYaw"),Y);
        auto N=P.Call(APawn::StaticClass(),Row?TEXT("AddControllerPitchInput"):TEXT("AddControllerYawInput"),420,Y);
        Wire(Pin(E,TEXT("then")),Pin(N,TEXT("execute")));Wire(Pin(E,TEXT("Value")),Pin(N,TEXT("Val")));
    }
    auto E=P.Event(TEXT("GraphSprint"),1000);auto B=P.Branch(Pin(E,TEXT("then")),Pin(E,TEXT("Enabled")),300,1000);
    auto Role=P.Branch(Pin(B,TEXT("then")),P.Get(TEXT("bHunter"),360,1200),650,1000);
    auto Speed=[&](UEdGraphPin* Exec,const TCHAR* Variable,int X,int Y){auto N=P.Set(TEXT("MaxWalkSpeed"),X,Y,UCharacterMovementComponent::StaticClass(),P.Movement(X-250,Y+150));Wire(Exec,Pin(N,TEXT("execute")));Wire(P.Get(Variable,X-250,Y+260),Pin(N,TEXT("MaxWalkSpeed")));};
    Speed(Pin(Role,TEXT("then")),TEXT("HunterSprintSpeed"),1100,950);
    Speed(Pin(Role,TEXT("else")),TEXT("RunnerSprintSpeed"),1100,1350);
    Speed(Pin(B,TEXT("else")),TEXT("WalkSpeed"),650,1800);
    P.Comment(TEXT("CAMERA — mouse input"),-50,-90,1000,900);
    P.Comment(TEXT("SPRINT — Shift sets role-specific speed; release restores walk speed"),-50,900,1650,1350);
}
static void JumpGraph(UBlueprint* BP){
    FPage P(BP,TEXT("06_Jump"));auto Jump=P.Event(TEXT("GraphJump"));
    auto Alive=P.Branch(Pin(Jump,TEXT("then")),P.Not(P.Get(TEXT("bOut"),0,250),250,250),500,0);
    auto Pressed=P.Branch(Pin(Alive,TEXT("then")),Pin(Jump,TEXT("Pressed")),800,0);
    auto Start=P.Call(ACharacter::StaticClass(),TEXT("Jump"),1100,0);Wire(Pin(Pressed,TEXT("then")),Pin(Start,TEXT("execute")));
    auto Stop=P.Call(ACharacter::StaticClass(),TEXT("StopJumping"),1100,350);Wire(Pin(Pressed,TEXT("else")),Pin(Stop,TEXT("execute")));
    P.Comment(TEXT("JUMP - Space press/release; eliminated characters cannot jump"),-50,-100,1600,750);
}
static void InteractionGraph(UBlueprint* BP){
    FPage P(BP,TEXT("03_Pickups_and_Escape"));
    auto E=P.Event(TEXT("GraphInteract"));
    auto Exec=P.Phase(P.Authority(Pin(E,TEXT("then")),300,0),650,0);
    auto Loop=P.Loop(Exec,PC(),true,2350,0);
    auto C=P.Cast(Pin(Loop,TEXT("LoopBody")),Pin(Loop,TEXT("Array Element")),PC(),2700,0);
    auto Item=C->GetCastResultPin();
    auto Near=P.Compare(TEXT("LessEqual_DoubleDouble"),P.Distance(Item,2900,220),P.Get(TEXT("PickupRange"),2900,340),3200,220);
    auto B=P.Branch(Pin(C,TEXT("then")),Near,3450,0);
    auto Role=P.Branch(Pin(B,TEXT("then")),P.Get(TEXT("bHunter"),3500,180),3750,0);
    auto IsKey=P.Get(TEXT("bKey"),3750,400,PC(),Item);
    auto HunterAllowed=P.And(P.Not(IsKey,3970,350),P.Not(P.Get(TEXT("bStick"),3750,520),3970,520),4190,350);
    auto RunnerAllowed=P.And(IsKey,P.Not(P.Get(TEXT("bKey"),3750,970),3970,970),4190,800);
    auto H=P.Branch(Pin(Role,TEXT("then")),HunterAllowed,4460,0);
    auto R=P.Branch(Pin(Role,TEXT("else")),RunnerAllowed,4460,700);
    auto GotStick=P.Assign(Pin(H,TEXT("then")),TEXT("bStick"),TEXT("true"),4750,0);
    auto GotKey=P.Assign(Pin(R,TEXT("then")),TEXT("bKey"),TEXT("true"),4750,700);
    auto Destroy=P.Call(AActor::StaticClass(),TEXT("K2_DestroyActor"),5050,0,Item);
    Wire(GotStick,Pin(Destroy,TEXT("execute")));Wire(GotKey,Pin(Destroy,TEXT("execute")));
    Wire(P.Refresh(Pin(Destroy,TEXT("then")),5350,0),Pin(Loop,TEXT("Break")));
    auto GameStateRef=P.State(2700,1550);
    auto Pos=P.Position(2700,1900);auto GatePoint=P.Get(TEXT("ExitLocation"),2700,2050);
    auto Dist=P.Math(TEXT("Vector_Distance2D"),3000,1900);Wire(Pos,Pin(Dist,TEXT("V1")));Wire(GatePoint,Pin(Dist,TEXT("V2")));
    auto Close=P.Compare(TEXT("LessEqual_DoubleDouble"),Pin(Dist,TEXT("ReturnValue")),P.Get(TEXT("ExitRange"),3000,2130),3250,1900);
    auto Allowed=P.And(Close,P.Not(P.Get(TEXT("bHunter"),3000,2300),3250,2300),3500,1950);
    auto AtGate=P.Branch(Pin(Loop,TEXT("Completed")),Allowed,3800,1550);
    auto HasKey=P.Branch(Pin(AtGate,TEXT("then")),P.Get(TEXT("bKey"),3800,1800),4100,1550);
    auto Unlock=P.Assign(Pin(HasKey,TEXT("then")),TEXT("bGateOpen"),TEXT("true"),4400,1550,SC(),GameStateRef);
    auto Open=P.Branch(Unlock,P.Get(TEXT("bGateOpen"),4400,1950,SC(),GameStateRef),4750,1550);
    Wire(Pin(HasKey,TEXT("else")),Pin(Open,TEXT("execute")));
    Exec=P.Assign(Pin(Open,TEXT("then")),TEXT("bEscaped"),TEXT("true"),5050,1550);
    Exec=P.Assign(Exec,TEXT("bOut"),TEXT("true"),5350,1550);
    auto Plus=P.Math(TEXT("Add_IntInt"),5350,1850);Wire(P.Get(TEXT("Escaped"),5050,1850,SC(),GameStateRef),Pin(Plus,TEXT("A")));Value(Pin(Plus,TEXT("B")),TEXT("1"));
    Exec=P.AssignPin(Exec,TEXT("Escaped"),Pin(Plus,TEXT("ReturnValue")),5650,1550,SC(),GameStateRef);
    P.Refresh(P.Stop(Exec,5950,1550),6250,1550);
    P.Comment(TEXT("E / SERVER — round active, player alive; iterate nearby pickups"),-50,-120,2650,750);
    P.Comment(TEXT("PICKUP RULES — hunter can take stick; runner can take key; remove collected actor"),2650,-120,3100,1400);
    P.Comment(TEXT("EXIT — key unlocks shared gate; each runner interacts to escape"),2650,1420,3950,1300);
}
static void AttackGraph(UBlueprint* BP){
    FPage P(BP,TEXT("04_Stick_Attack"));auto E=P.Event(TEXT("GraphAttack"));
    auto Exec=P.Phase(P.Authority(Pin(E,TEXT("then")),300,0),650,0);
    auto Armed=P.And(P.Get(TEXT("bHunter"),1900,270),P.Get(TEXT("bStick"),1900,400),2150,280);
    auto Sub=P.Math(TEXT("Subtract_DoubleDouble"),2150,600);Wire(P.Time(1850,550),Pin(Sub,TEXT("A")));Wire(P.Get(TEXT("LastSwing"),1850,700),Pin(Sub,TEXT("B")));
    auto Ready=P.Compare(TEXT("GreaterEqual_DoubleDouble"),Pin(Sub,TEXT("ReturnValue")),P.Get(TEXT("AttackCooldown"),2150,800),2400,600);
    auto B=P.Branch(Exec,P.And(Armed,Ready,2670,300),2950,0);
    Exec=P.AssignPin(Pin(B,TEXT("then")),TEXT("LastSwing"),P.Time(3000,250),3250,0);
    auto L=P.Loop(Exec,CC(),false,3850,0);
    auto C=P.Cast(Pin(L,TEXT("LoopBody")),Pin(L,TEXT("Array Element")),CC(),4150,0);auto Target=C->GetCastResultPin();
    auto Alive=P.Not(P.Get(TEXT("bOut"),4150,350,CC(),Target),4400,350);
    auto Runner=P.Not(P.Get(TEXT("bHunter"),4150,500,CC(),Target),4400,500);
    auto Dist=P.Compare(TEXT("LessEqual_DoubleDouble"),P.Distance(Target,4400,750),P.Get(TEXT("AttackRange"),4400,900),4650,750);
    auto Candidate=P.Branch(Pin(C,TEXT("then")),P.And(P.And(Alive,Runner,4650,350),Dist,4900,350),5200,0);
    auto Delta=P.Math(TEXT("Subtract_VectorVector"),4900,1100);Wire(P.Position(4550,1080,Target),Pin(Delta,TEXT("A")));Wire(P.Position(4550,1250),Pin(Delta,TEXT("B")));
    auto Normal=P.Math(TEXT("Normal"),5150,1100);Wire(Pin(Delta,TEXT("ReturnValue")),Pin(Normal,TEXT("A")));
    auto Dot=P.Math(TEXT("Dot_VectorVector"),5400,1100);Wire(Pin(Normal,TEXT("ReturnValue")),Pin(Dot,TEXT("A")));Wire(Pin(P.Call(AActor::StaticClass(),TEXT("GetActorForwardVector"),5100,1400),TEXT("ReturnValue")),Pin(Dot,TEXT("B")));
    auto Facing=P.Compare(TEXT("GreaterEqual_DoubleDouble"),Pin(Dot,TEXT("ReturnValue")),nullptr,5650,1100,TEXT("0.25"));
    auto Front=P.Branch(Then(Candidate),Facing,5800,0);
    auto Line=P.Call(UKismetSystemLibrary::StaticClass(),TEXT("LineTraceSingle"),6100,0);
    Wire(Then(Front),Pin(Line,TEXT("execute")));Wire(P.Position(5800,1500),Pin(Line,TEXT("Start")));Wire(P.Position(5800,1700,Target),Pin(Line,TEXT("End")));
    Value(Pin(Line,TEXT("TraceChannel")),TEXT("TraceTypeQuery1"));Value(Pin(Line,TEXT("bIgnoreSelf")),TEXT("true"));
    auto Ignore=P.Node<UK2Node_MakeArray>(5800,1900);Ignore->AllocateDefaultPins();Wire(P.Self(5500,1900),Pin(Ignore,TEXT("[0]")));Wire(Pin(Ignore,TEXT("Array")),Pin(Line,TEXT("ActorsToIgnore")));
    auto Break=P.Call(UGameplayStatics::StaticClass(),TEXT("BreakHitResult"),6400,1000);Wire(Pin(Line,TEXT("OutHit")),Pin(Break,TEXT("Hit")));
    auto Same=P.Op(TEXT("EqualEqual_ObjectObject"),Pin(Break,TEXT("HitActor")),Target,6700,1200);
    auto Hit=P.Branch(Then(Line),P.Or(P.Not(Out(Line),6400,1500),Same,7000,1300),7300,0);
    auto Hurt=P.Call(CC(),TEXT("GraphReceiveHit"),7650,0,Target);Wire(Pin(Hit,TEXT("then")),Pin(Hurt,TEXT("execute")));
    P.Comment(TEXT("ATTACK / SERVER — active round, hunter has stick, attack cooldown"),-50,-120,3550,1150);
    P.Comment(TEXT("HIT VALIDATION — living runner, distance, forward cone, no wall in between; call target damage graph"),3500,-120,3350,2050);
}
static void DamageGraph(UBlueprint* BP){
    FPage P(BP,TEXT("05_Three_Lives_and_Death"));auto E=P.Event(TEXT("GraphReceiveHit"));
    auto Exec=P.Authority(Pin(E,TEXT("then")),300,0);
    auto Sub=P.Math(TEXT("Subtract_DoubleDouble"),650,320);Wire(P.Time(350,300),Pin(Sub,TEXT("A")));Wire(P.Get(TEXT("LastHit"),350,450),Pin(Sub,TEXT("B")));
    auto Ready=P.Compare(TEXT("GreaterEqual_DoubleDouble"),Pin(Sub,TEXT("ReturnValue")),P.Get(TEXT("HitInvulnerability"),650,550),900,320);
    auto Allowed=P.And(Ready,P.Not(P.Get(TEXT("bOut"),900,650),1150,650),1400,350);
    auto B=P.Branch(Exec,Allowed,1650,0);
    Exec=P.AssignPin(Pin(B,TEXT("then")),TEXT("LastHit"),P.Time(1650,250),1950,0);
    auto Minus=P.Math(TEXT("Subtract_IntInt"),1950,350);Wire(P.Get(TEXT("Lives"),1650,380),Pin(Minus,TEXT("A")));Value(Pin(Minus,TEXT("B")),TEXT("1"));
    auto Clamp=P.Math(TEXT("Max"),2200,350);Wire(Pin(Minus,TEXT("ReturnValue")),Pin(Clamp,TEXT("A")));Value(Pin(Clamp,TEXT("B")),TEXT("0"));
    Exec=P.AssignPin(Exec,TEXT("Lives"),Pin(Clamp,TEXT("ReturnValue")),2450,0);
    auto Dead=P.Branch(Exec,P.Compare(TEXT("LessEqual_IntInt"),P.Get(TEXT("Lives"),2450,350),nullptr,2700,350,TEXT("0")),2950,0);
    Exec=P.Assign(Pin(Dead,TEXT("then")),TEXT("bOut"),TEXT("true"),3250,0);
    Exec=P.Stop(Exec,3550,0);
    auto Key=P.Branch(Exec,P.Get(TEXT("bKey"),3550,330),3900,0);
    auto Spawn=P.Node<UK2Node_SpawnActorFromClass>(4200,0);Spawn->AllocateDefaultPins();Pin(Spawn,TEXT("Class"))->DefaultObject=PC();Spawn->PinDefaultValueChanged(Pin(Spawn,TEXT("Class")));
    Wire(Pin(Key,TEXT("then")),Pin(Spawn,TEXT("execute")));
    auto Offset=P.Math(TEXT("Add_VectorVector"),3900,650);Wire(P.Position(3550,650),Pin(Offset,TEXT("A")));Value(Pin(Offset,TEXT("B")),TEXT("0,0,-20"));
    auto Transform=P.Math(TEXT("MakeTransform"),4200,650);Wire(Pin(Offset,TEXT("ReturnValue")),Pin(Transform,TEXT("Location")));Wire(Pin(Transform,TEXT("ReturnValue")),Pin(Spawn,TEXT("SpawnTransform")));
    Value(Pin(Spawn,TEXT("CollisionHandlingOverride")),TEXT("AlwaysSpawn"));
    auto Dropped=Pin(Spawn,TEXT("ReturnValue"));
    Exec=P.Assign(Pin(Spawn,TEXT("then")),TEXT("bKey"),TEXT("true"),4550,0,PC(),Dropped);
    auto Appearance=P.Call(PC(),TEXT("UpdateAppearance"),4850,0,Dropped);Wire(Exec,Pin(Appearance,TEXT("execute")));
    Exec=P.Assign(Pin(Appearance,TEXT("then")),TEXT("bKey"),TEXT("false"),5150,0);
    auto Refresh=P.Call(CC(),TEXT("RefreshLook"),5450,0);Wire(Exec,Pin(Refresh,TEXT("execute")));Wire(Pin(Key,TEXT("else")),Pin(Refresh,TEXT("execute")));
    P.Comment(TEXT("RECEIVE HIT / SERVER — invulnerability window; subtract one life, clamp at zero"),-50,-120,3150,1150);
    P.Comment(TEXT("THIRD HIT — eliminate, stop movement, drop carried key, update appearance and collision"),3150,-120,2700,1150);
}
void BuildCharacterLogic(){MovementGraphs(Character);CameraSprintGraphs(Character);JumpGraph(Character);InteractionGraph(Character);AttackGraph(Character);DamageGraph(Character);}
}
