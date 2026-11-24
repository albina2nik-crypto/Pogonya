#include "PureBlueprintTools.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInterface.h"

namespace PureGame {
static UEdGraphPin* Visibility(FPage& P,UEdGraphPin* E,const TCHAR* Component,UEdGraphPin* Show,int X,int Y){auto N=P.Call(USceneComponent::StaticClass(),TEXT("SetVisibility"),X,Y,P.Get(Component,X-250,Y+200));Wire(E,Pin(N,TEXT("execute")));Wire(Show,Pin(N,TEXT("bNewVisibility")));Value(Pin(N,TEXT("bPropagateToChildren")),TEXT("true"));return Then(N);}
static UEdGraphPin* Rotation(FPage& P,UEdGraphPin* E,const TCHAR* Component,UEdGraphPin* Angle,int X,int Y){auto R=P.Math(TEXT("MakeRotator"),X-250,Y+180);Wire(Angle,Pin(R,TEXT("Pitch")));auto N=P.Call(USceneComponent::StaticClass(),TEXT("K2_SetRelativeRotation"),X,Y,P.Get(Component,X-250,Y+400));Wire(E,Pin(N,TEXT("execute")));Wire(Out(R),Pin(N,TEXT("NewRotation")));return Then(N);}
void BuildCharacterVisuals(){
    FPage P(Character,TEXT("07_Appearance"));auto E=P.Event(TEXT("RefreshLook"));auto Hunter=P.Get(TEXT("bHunter"),0,400);
    auto H=P.Obj(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Pogonya/Materials/M_Hunter")),UMaterialInterface::StaticClass(),0,650);
    auto R=P.Obj(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Pogonya/Materials/M_Runner")),UMaterialInterface::StaticClass(),0,850);
    auto Cloth=P.Choose(Hunter,H,R,400,500);auto Exec=Then(E);
    int X=750;for(auto Name:{TEXT("Body"),TEXT("LeftArm"),TEXT("RightArm")}){auto M=P.Call(UMeshComponent::StaticClass(),TEXT("SetMaterial"),X,0,P.Get(Name,X,250));Wire(Exec,Pin(M,TEXT("execute")));Wire(Cloth,Pin(M,TEXT("Material")));Exec=Then(M);X+=350;}
    auto OutState=P.Get(TEXT("bOut"),1800,350);Exec=Visibility(P,Exec,TEXT("Weapon"),P.And(P.Get(TEXT("bStick"),1800,550),P.Not(OutState,2100,350),2400,400),2700,0);
    auto B=P.Branch(Exec,OutState,3150,0);
    auto Collision=P.Call(UPrimitiveComponent::StaticClass(),TEXT("SetCollisionEnabled"),3500,850,P.Get(TEXT("CapsuleComponent"),3150,1200));Wire(Pin(B,TEXT("else")),Pin(Collision,TEXT("execute")));
    // Collision enum is selected by branch to keep the graph obvious to students.
    Value(Pin(Collision,TEXT("NewType")),TEXT("QueryAndPhysics"));
    auto Disable=P.Call(UPrimitiveComponent::StaticClass(),TEXT("SetCollisionEnabled"),3850,0,P.Get(TEXT("CapsuleComponent"),3550,600));Wire(Then(B),Pin(Disable,TEXT("execute")));Value(Pin(Disable,TEXT("NewType")),TEXT("NoCollision"));
    auto Halt=P.Do(Then(Disable),UCharacterMovementComponent::StaticClass(),TEXT("DisableMovement"),4200,0,P.Movement(3850,350));
    auto Pose=P.Call(USceneComponent::StaticClass(),TEXT("K2_SetRelativeRotation"),4650,0,P.Get(TEXT("Figure"),4300,650));Wire(Halt,Pin(Pose,TEXT("execute")));Wire(Then(Collision),Pin(Pose,TEXT("execute")));
    auto VRot=Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FRotator>::Get());Wire(P.Choose(OutState,P.Literal(VRot,TEXT("0,0,85"),3500,1000),P.Literal(VRot,TEXT("0,0,0"),3500,1250),3900,1000),Pin(Pose,TEXT("NewRotation")));
    Visibility(P,Then(Pose),TEXT("Figure"),P.Not(P.Get(TEXT("bEscaped"),4300,600),4600,600),4900,0);
    P.Comment(TEXT("ВНЕШНИЙ ВИД — цвет роли, палка в руке, поза выбывания и исчезновение спасшегося. Все компоненты редактируются в Blueprint."),-80,-140,5400,1650);

    FPage A(Character,TEXT("08_Tick_and_Animation"));auto Tick=A.NativeEvent(AActor::StaticClass(),TEXT("ReceiveTick"));auto Seq=A.Sequence(Then(Tick),350,0,3);
    A.Do(Seq->GetThenPinGivenIndex(0),CC(),TEXT("RefreshLook"),700,0);
    auto Active=A.Branch(Seq->GetThenPinGivenIndex(1),A.Not(A.Get(TEXT("bOut"),400,650),700,650),1000,450);
    auto S=A.State(0,1150);auto Valid=A.Branch(Then(Active),A.Valid(S,600,1150),1350,450);
    auto Time=Out(A.Call(AGameStateBase::StaticClass(),TEXT("GetServerWorldTimeSeconds"),950,1150,S));
    auto Speed=A.Math(TEXT("VSizeXY"),950,1450);Wire(Out(A.Call(AActor::StaticClass(),TEXT("GetVelocity"),600,1450)),Pin(Speed,TEXT("A")));
    auto Clamp=A.Math(TEXT("FClamp"),1550,1450);Wire(A.Op(TEXT("Divide_DoubleDouble"),Out(Speed),nullptr,1250,1450,TEXT("500")),Pin(Clamp,TEXT("Value")));Value(Pin(Clamp,TEXT("Max")),TEXT("1"));
    auto Sin=A.Math(TEXT("Sin"),1550,1150);Wire(A.Op(TEXT("Multiply_DoubleDouble"),Time,nullptr,1250,1150,TEXT("11")),Pin(Sin,TEXT("A")));
    auto Swing=A.Op(TEXT("Multiply_DoubleDouble"),A.Op(TEXT("Multiply_DoubleDouble"),Out(Sin),Out(Clamp),1850,1200),nullptr,2150,1200,TEXT("28"));
    auto Neg=A.Op(TEXT("Multiply_DoubleDouble"),Swing,nullptr,2450,1500,TEXT("-1"));
    Exec=Rotation(A,Then(Valid),TEXT("LeftLeg"),Swing,2600,450);Exec=Rotation(A,Exec,TEXT("RightLeg"),Neg,3000,450);Exec=Rotation(A,Exec,TEXT("LeftArm"),Neg,3400,450);
    auto Elapsed=A.Op(TEXT("Subtract_DoubleDouble"),Time,A.Get(TEXT("LastSwing"),2450,2000),2750,2000);
    auto Attacking=A.Compare(TEXT("Less_DoubleDouble"),Elapsed,nullptr,3050,2000,TEXT("0.35"));
    auto ArmAngle=A.Op(TEXT("Add_DoubleDouble"),A.Op(TEXT("Multiply_DoubleDouble"),Elapsed,nullptr,3350,2000,TEXT("260")),nullptr,3650,2000,TEXT("-95"));
    Exec=Rotation(A,Exec,TEXT("RightArm"),A.Choose(Attacking,ArmAngle,Swing,3950,1700),4300,450);
    auto BatAngle=A.Op(TEXT("Add_DoubleDouble"),A.Op(TEXT("Multiply_DoubleDouble"),Elapsed,nullptr,3650,2400,TEXT("360")),nullptr,3950,2400,TEXT("-75"));
    Rotation(A,Exec,TEXT("Weapon"),A.Choose(Attacking,BatAngle,A.Num(35,3950,2700),4300,2400),4750,450);
    auto Server=A.Authority(Seq->GetThenPinGivenIndex(2),700,3250);auto Bot=A.Branch(Server,A.Get(TEXT("bBot"),950,3500),1200,3250);auto Phase=A.Phase(Then(Bot),1550,3250);
    auto Due=A.Branch(Phase,A.Compare(TEXT("GreaterEqual_DoubleDouble"),A.Time(2800,4200),A.Get(TEXT("NextDecision"),2800,4400),3150,4200),3500,3250);
    auto Decided=A.Do(Then(Due),CC(),TEXT("BotDecide"),3850,3250);auto Move=A.Call(CC(),TEXT("BotMove"),4200,3250);Wire(Decided,Pin(Move,TEXT("execute")));Wire(Pin(Due,TEXT("else")),Pin(Move,TEXT("execute")));
    A.Comment(TEXT("АНИМАЦИЯ — синус движения рук и ног; отдельный взмах палкой. Синхронизация по времени сервера."),-80,-140,5400,3050);
    A.Comment(TEXT("БОТЫ — решения каждые 0,35 с; движение каждый кадр только на сервере"),500,3100,4300,1600);
}
void BuildPickupGateLogic(){
    FPage P(Pickup,TEXT("01_Type_and_Appearance"));auto E=P.Event(TEXT("UpdateAppearance"));auto Key=P.Get(TEXT("bKey"),0,400);auto Exec=Visibility(P,Then(E),TEXT("StickMesh"),P.Not(Key,300,400),600,0);
    int X=1000;for(auto N:{TEXT("KeyRing"),TEXT("KeyShaft"),TEXT("KeyTooth")}){Exec=Visibility(P,Exec,N,Key,X,0);X+=400;}
    FPage T(Pickup,TEXT("02_Rotation"));auto Tick=T.NativeEvent(AActor::StaticClass(),TEXT("ReceiveTick"));auto Seq=T.Sequence(Then(Tick),350,0);
    T.Do(Seq->GetThenPinGivenIndex(0),PC(),TEXT("UpdateAppearance"),700,0);
    auto Rot=T.Math(TEXT("MakeRotator"),700,500);Wire(T.Op(TEXT("Multiply_DoubleDouble"),Pin(Tick,TEXT("DeltaSeconds")),nullptr,350,500,TEXT("32")),Pin(Rot,TEXT("Yaw")));
    auto Turn=T.Call(AActor::StaticClass(),TEXT("K2_AddActorLocalRotation"),1050,350);Wire(Seq->GetThenPinGivenIndex(1),Pin(Turn,TEXT("execute")));Wire(Out(Rot),Pin(Turn,TEXT("DeltaRotation")));
    P.Comment(TEXT("ТИП ПРЕДМЕТА — bKey выбирает ключ или палку. Подсветка помогает найти предмет в темноте."),-80,-140,2600,950);

    FPage G(Gate,TEXT("01_Open_Gate_Animation"));auto TickG=G.NativeEvent(AActor::StaticClass(),TEXT("ReceiveTick"));auto S=G.State(0,400);auto Valid=G.Branch(Then(TickG),G.Valid(S,550,400),850,0);
    auto Target=G.Choose(G.Get(TEXT("bGateOpen"),850,450,SC(),S),G.Num(1,850,650),G.Num(0,850,850),1200,450);
    auto Interp=G.Math(TEXT("FInterpTo"),1550,450);Wire(G.Get(TEXT("OpenAmount"),1200,1000),Pin(Interp,TEXT("Current")));Wire(Target,Pin(Interp,TEXT("Target")));Wire(Pin(TickG,TEXT("DeltaSeconds")),Pin(Interp,TEXT("DeltaTime")));Value(Pin(Interp,TEXT("InterpSpeed")),TEXT("2.2"));
    Exec=G.AssignPin(Then(Valid),TEXT("OpenAmount"),Out(Interp),1900,0);
    for(int i=0;i<2;i++){
        int XX=2300+i*1350;auto Offset=G.Op(TEXT("Multiply_DoubleDouble"),G.Get(TEXT("OpenAmount"),XX,500),nullptr,XX+300,500,i?TEXT("620"):TEXT("-620"));auto PosX=G.Op(TEXT("Add_DoubleDouble"),Offset,nullptr,XX+600,500,i?TEXT("300"):TEXT("-300"));
        auto Vec=G.Math(TEXT("MakeVector"),XX+900,500);Wire(PosX,Pin(Vec,TEXT("X")));Value(Pin(Vec,TEXT("Z")),TEXT("255"));
        auto Move=G.Call(USceneComponent::StaticClass(),TEXT("K2_SetRelativeLocation"),XX+900,0,G.Get(i?TEXT("RightDoor"):TEXT("LeftDoor"),XX+600,900));Wire(Exec,Pin(Move,TEXT("execute")));Wire(Out(Vec),Pin(Move,TEXT("NewLocation")));Exec=Then(Move);
    }
    G.Comment(TEXT("ВОРОТА — bGateOpen плавно раздвигает две створки. При новом раунде створки возвращаются обратно."),-80,-140,5200,1450);
}
}
