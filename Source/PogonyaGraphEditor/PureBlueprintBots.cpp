#include "PureBlueprintTools.h"
namespace PureGame {
static UEdGraphPin* Target(FPage& P,UEdGraphPin* E,UEdGraphPin* Pos,int X,int Y){auto Next=P.AssignPin(E,TEXT("BotTarget"),Pos,X,Y);return P.Assign(Next,TEXT("BotFoundTarget"),TEXT("true"),X+320,Y);}
static void Decide(){
    FPage P(Character,TEXT("09_Bot_Decision"));auto E=P.Event(TEXT("BotDecide"));
    auto Exec=P.AssignPin(Then(E),TEXT("NextDecision"),P.Op(TEXT("Add_DoubleDouble"),P.Time(0,350),nullptr,300,350,TEXT("0.35")),650,0);
    Exec=P.Assign(Exec,TEXT("BestDistance"),TEXT("999999"),950,0);Exec=P.Assign(Exec,TEXT("BotThreat"),TEXT("None"),1250,0);Exec=P.Assign(Exec,TEXT("BotFoundTarget"),TEXT("false"),1550,0);
    auto L=P.Loop(Exec,CC(),false,2150,0);auto C=P.Cast(Pin(L,TEXT("LoopBody")),Pin(L,TEXT("Array Element")),CC(),2500,0);auto Ch=C->GetCastResultPin();
    auto Enemy=P.Op(TEXT("NotEqual_BoolBool"),P.Get(TEXT("bHunter"),2500,350),P.Get(TEXT("bHunter"),2500,550,CC(),Ch),2800,350);
    auto Alive=P.Not(P.Get(TEXT("bOut"),2500,750,CC(),Ch),2800,750);auto Good=P.Branch(Then(C),P.And(Enemy,Alive,3150,450),3500,0);
    auto Dist=P.Distance(Ch,3500,400);auto Near=P.Branch(Then(Good),P.Compare(TEXT("Less_DoubleDouble"),Dist,P.Get(TEXT("BestDistance"),3500,650),3850,450),4200,0);
    Exec=P.AssignPin(Then(Near),TEXT("BestDistance"),Dist,4550,0);P.AssignPin(Exec,TEXT("BotThreat"),Ch,4900,0);
    auto Threat=P.Get(TEXT("BotThreat"),2500,1500);auto CanChase=P.And(P.And(P.Get(TEXT("bHunter"),2500,1750),P.Get(TEXT("bStick"),2500,2000),2800,1750),P.Valid(Threat,2800,1500),3150,1550);
    auto Chase=P.Branch(Pin(L,TEXT("Completed")),CanChase,3500,1250);auto ChaseDone=Target(P,Then(Chase),P.Position(3500,1750,Threat),3850,1250);
    auto S=P.State(2500,2500);auto Escape=P.And(P.Not(P.Get(TEXT("bHunter"),3500,2350),3800,2350),P.Or(P.Get(TEXT("bKey"),3500,2650),P.Get(TEXT("bGateOpen"),3500,2900,SC(),S),3800,2650),4150,2450);
    auto Exit=P.Branch(ChaseDone,Escape,4550,2000);Wire(Pin(Chase,TEXT("else")),Pin(Exit,TEXT("execute")));
    auto ExitDone=Target(P,Then(Exit),P.Get(TEXT("ExitLocation"),4550,2450),4900,2000);
    auto NeedItem=P.Branch(ExitDone,P.Not(P.Get(TEXT("BotFoundTarget"),4900,2950),5200,2950),5550,2550);Wire(Pin(Exit,TEXT("else")),Pin(NeedItem,TEXT("execute")));
    auto Items=P.Loop(Then(NeedItem),PC(),true,6150,2550);auto Item=P.Cast(Pin(Items,TEXT("LoopBody")),Pin(Items,TEXT("Array Element")),PC(),6500,2550);auto It=Item->GetCastResultPin();
    auto Match=P.Branch(Then(Item),P.Op(TEXT("NotEqual_BoolBool"),P.Get(TEXT("bKey"),6500,2950,PC(),It),P.Get(TEXT("bHunter"),6500,3200),6850,2950),7200,2550);
    Wire(Target(P,Then(Match),P.Position(7200,3050,It),7550,2550),Pin(Items,TEXT("Break")));
    auto ValidThreat=P.Branch(Pin(Items,TEXT("Completed")),P.Valid(Threat,6500,4000),6850,3600);Wire(Pin(NeedItem,TEXT("else")),Pin(ValidThreat,TEXT("execute")));
    auto Danger=P.And(P.Not(P.Get(TEXT("bHunter"),6850,4350),7200,4350),P.Compare(TEXT("Less_DoubleDouble"),P.Get(TEXT("BestDistance"),6850,4600),nullptr,7200,4600,TEXT("650")),7550,4350);
    auto Flee=P.Branch(Then(ValidThreat),Danger,7900,3600);
    auto Away=P.Op(TEXT("Subtract_VectorVector"),P.Position(7900,4100),P.Position(7900,4350,Threat),8250,4100);auto Normal=P.Math(TEXT("Vector_Normal2D"),8600,4100);Wire(Away,Pin(Normal,TEXT("A")));
    auto Delta=P.Op(TEXT("Multiply_VectorFloat"),Out(Normal),nullptr,8950,4100,TEXT("700"));auto RunTo=P.Op(TEXT("Add_VectorVector"),P.Position(8600,4500),Delta,9300,4100);
    auto Fled=Target(P,Then(Flee),RunTo,9650,3600);
    auto Wander=P.Branch(Fled,P.Not(P.Get(TEXT("BotFoundTarget"),9650,4900),9950,4900),10300,4550);Wire(Pin(Flee,TEXT("else")),Pin(Wander,TEXT("execute")));Wire(Pin(ValidThreat,TEXT("else")),Pin(Wander,TEXT("execute")));
    auto DistTarget=P.Math(TEXT("Vector_Distance2D"),10300,5050);Wire(P.Position(9950,5200),Pin(DistTarget,TEXT("V1")));Wire(P.Get(TEXT("BotTarget"),9950,5450),Pin(DistTarget,TEXT("V2")));
    auto Arrived=P.Branch(Then(Wander),P.Compare(TEXT("Less_DoubleDouble"),Out(DistTarget),nullptr,10650,5050,TEXT("150")),11000,4550);
    auto RX=P.Math(TEXT("RandomFloatInRange"),11000,5200);Value(Pin(RX,TEXT("Min")),TEXT("-1900"));Value(Pin(RX,TEXT("Max")),TEXT("1900"));
    auto RY=P.Math(TEXT("RandomFloatInRange"),11000,5450);Value(Pin(RY,TEXT("Min")),TEXT("-2300"));Value(Pin(RY,TEXT("Max")),TEXT("2300"));auto Vec=P.Math(TEXT("MakeVector"),11350,5200);Wire(Out(RX),Pin(Vec,TEXT("X")));Wire(Out(RY),Pin(Vec,TEXT("Y")));Value(Pin(Vec,TEXT("Z")),TEXT("100"));
    auto Chosen=P.AssignPin(Then(Arrived),TEXT("BotTarget"),Out(Vec),11700,4550);
    auto Interact=P.Call(CC(),TEXT("GraphInteract"),12050,4550);Wire(Chosen,Pin(Interact,TEXT("execute")));Wire(Pin(Arrived,TEXT("else")),Pin(Interact,TEXT("execute")));Wire(Pin(Wander,TEXT("else")),Pin(Interact,TEXT("execute")));
    Exec=P.Do(Then(Interact),CC(),TEXT("GraphAttack"),12400,4550);auto Sprint=P.Call(CC(),TEXT("GraphSprint"),12750,4550);Wire(Exec,Pin(Sprint,TEXT("execute")));Value(Pin(Sprint,TEXT("Enabled")),TEXT("true"));
    P.Comment(TEXT("1 — Найти ближайшего активного противника; один цикл решений каждые 0,35 секунды"),-80,-150,5600,1150);
    P.Comment(TEXT("2 — Гонщик с палкой преследует; беглец с ключом идёт к выходу; иначе поиск предмета"),2350,1120,6100,2300);
    P.Comment(TEXT("3 — Убежать от близкого гонщика, подобрать предмет, атаковать или выбрать точку движения"),6350,3450,7000,2500);
}
static void Move(){
    FPage P(Character,TEXT("10_Bot_Steering"));auto E=P.Event(TEXT("BotMove"));auto Exec=P.Assign(Then(E),TEXT("BestSteerScore"),TEXT("-999999"),350,0);
    auto Desired=P.Math(TEXT("Vector_Normal2D"),700,650);Wire(P.Op(TEXT("Subtract_VectorVector"),P.Get(TEXT("BotTarget"),0,650),P.Position(0,900),350,650),Pin(Desired,TEXT("A")));
    auto L=P.Macro(TEXT("ForLoop"),700,0);Wire(Exec,Pin(L,TEXT("execute")));Value(Pin(L,TEXT("LastIndex")),TEXT("7"));
    auto Ang=P.Node<UK2Node_Select>(1050,650);Ang->AllocateDefaultPins();for(int i=0;i<6;i++)Ang->AddInputPin();Out(Ang)->PinType=Type(UEdGraphSchema_K2::PC_Real);Ang->PinTypeChanged(Out(Ang));Wire(Pin(L,TEXT("Index")),Pin(Ang,TEXT("Index")));TArray<UEdGraphPin*> Opt;Ang->GetOptionPins(Opt);const TCHAR* Angles[]={TEXT("0"),TEXT("35"),TEXT("-35"),TEXT("70"),TEXT("-70"),TEXT("110"),TEXT("-110"),TEXT("160")};for(int i=0;i<Opt.Num();i++)Value(Opt[i],Angles[i]);
    auto Dir=P.Math(TEXT("RotateAngleAxis"),1400,650);Wire(Out(Desired),Pin(Dir,TEXT("InVect")));Wire(Out(Ang),Pin(Dir,TEXT("AngleDeg")));Value(Pin(Dir,TEXT("Axis")),TEXT("0,0,1"));
    auto End=P.Op(TEXT("Add_VectorVector"),P.Position(1400,1250),P.Op(TEXT("Multiply_VectorFloat"),Out(Dir),nullptr,1750,1000,TEXT("200")),2100,1000);
    auto Trace=P.Call(UKismetSystemLibrary::StaticClass(),TEXT("SphereTraceSingle"),2450,0);Wire(Pin(L,TEXT("LoopBody")),Pin(Trace,TEXT("execute")));Wire(P.Position(2100,1300),Pin(Trace,TEXT("Start")));Wire(End,Pin(Trace,TEXT("End")));Value(Pin(Trace,TEXT("Radius")),TEXT("40"));Value(Pin(Trace,TEXT("TraceChannel")),TEXT("TraceTypeQuery1"));Value(Pin(Trace,TEXT("bIgnoreSelf")),TEXT("true"));
    auto Ignore=P.Node<UK2Node_MakeArray>(1750,1700);Ignore->AllocateDefaultPins();Wire(P.Self(1400,1700),Pin(Ignore,TEXT("[0]")));Wire(Pin(Ignore,TEXT("Array")),Pin(Trace,TEXT("ActorsToIgnore")));
    auto Hit=P.Call(UGameplayStatics::StaticClass(),TEXT("BreakHitResult"),2800,800);Wire(Pin(Trace,TEXT("OutHit")),Pin(Hit,TEXT("Hit")));
    auto Score=P.Op(TEXT("Add_DoubleDouble"),P.Choose(Out(Trace),P.Op(TEXT("Multiply_DoubleDouble"),Pin(Hit,TEXT("Time")),nullptr,3150,800,TEXT("0.7")),P.Num(1.5,3150,1050),3500,800),P.Op(TEXT("Multiply_DoubleDouble"),P.Op(TEXT("Dot_VectorVector"),Out(Dir),Out(Desired),3150,1450),nullptr,3500,1450,TEXT("0.5")),3850,1000);
    auto Better=P.Branch(Then(Trace),P.Compare(TEXT("Greater_DoubleDouble"),Score,P.Get(TEXT("BestSteerScore"),3850,1450),4200,1100),4550,0);
    Exec=P.AssignPin(Then(Better),TEXT("BestSteerScore"),Score,4900,0);P.AssignPin(Exec,TEXT("BotSteer"),Out(Dir),5250,0);
    auto MoveNode=P.Call(APawn::StaticClass(),TEXT("AddMovementInput"),4900,2000);Wire(Pin(L,TEXT("Completed")),Pin(MoveNode,TEXT("execute")));Wire(P.Get(TEXT("BotSteer"),4550,2400),Pin(MoveNode,TEXT("WorldDirection")));Value(Pin(MoveNode,TEXT("ScaleValue")),TEXT("1"));
    P.Comment(TEXT("ОБХОД ПРЕГРАД — проверить восемь направлений сферической трассировкой; выбрать свободное направление ближе к цели"),-80,-150,5800,2950);
}
void BuildBotLogic(){Decide();Move();}
}
