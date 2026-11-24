#include "PureBlueprintTools.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace PureGame {
static UEdGraphPin* Join(FPage& P,UEdGraphPin* A,UEdGraphPin* B,int X,int Y){auto N=P.Call(UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"),X,Y);Wire(A,Pin(N,TEXT("A")));Wire(B,Pin(N,TEXT("B")));return Out(N);}
static UEdGraphPin* IntegerString(FPage& P,UEdGraphPin* I,int X,int Y){auto N=P.Call(UKismetStringLibrary::StaticClass(),TEXT("Conv_IntToString"),X,Y);Wire(I,Pin(N,TEXT("InInt")));return Out(N);}
static UEdGraphPin* Color(FPage& P,const TCHAR* C,int X,int Y){return P.Literal(Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FLinearColor>::Get()),C,X,Y);}
struct FDraw {
    FPage& P;UEdGraphPin* Scale;int X=1300,Y=0;
    UEdGraphPin* Scaled(float V){return P.Op(TEXT("Multiply_DoubleDouble"),Scale,nullptr,X-300,Y+220,*FString::SanitizeFloat(V));}
    UEdGraphPin* Rect(UEdGraphPin* E,float PX,float PY,float W,float H,const TCHAR* Col,UEdGraphPin* DynamicColor=nullptr,UEdGraphPin* Width=nullptr){
        auto N=P.Call(AHUD::StaticClass(),TEXT("DrawRect"),X,Y);Wire(E,Pin(N,TEXT("execute")));
        if(DynamicColor)Wire(DynamicColor,Pin(N,TEXT("RectColor")));else Value(Pin(N,TEXT("RectColor")),Col);
        Wire(Scaled(PX),Pin(N,TEXT("ScreenX")));Wire(Scaled(PY),Pin(N,TEXT("ScreenY")));Wire(Width?Width:Scaled(W),Pin(N,TEXT("ScreenW")));Wire(Scaled(H),Pin(N,TEXT("ScreenH")));X+=350;return Then(N);
    }
    UEdGraphPin* Text(UEdGraphPin* E,UEdGraphPin* Txt,float PX,float PY,float FontScale,const TCHAR* Col=TEXT("(R=0.93,G=0.96,B=1,A=1)"),UEdGraphPin* DynamicColor=nullptr){
        auto N=P.Call(AHUD::StaticClass(),TEXT("DrawText"),X,Y);Wire(E,Pin(N,TEXT("execute")));Wire(Txt,Pin(N,TEXT("Text")));
        if(DynamicColor)Wire(DynamicColor,Pin(N,TEXT("TextColor")));else Value(Pin(N,TEXT("TextColor")),Col);
        Wire(Scaled(PX),Pin(N,TEXT("ScreenX")));Wire(Scaled(PY),Pin(N,TEXT("ScreenY")));Wire(Scaled(FontScale),Pin(N,TEXT("Scale")));Pin(N,TEXT("Font"))->DefaultObject=GEngine->GetMediumFont();X+=350;return Then(N);
    }
};
void BuildHUDLogic(){
    FPage P(HUD,TEXT("01_Draw_Interface"));auto E=P.NativeEvent(AHUD::StaticClass(),TEXT("ReceiveDrawHUD"));auto Pawn=P.Call(AHUD::StaticClass(),TEXT("GetOwningPawn"),0,400);auto C=P.Cast(Then(E),Out(Pawn),CC(),350,0);auto Ch=C->GetCastResultPin();auto S=P.State(0,750);auto Valid=P.Branch(Then(C),P.Valid(S,550,750),850,0);
    auto SX=P.Math(TEXT("Conv_IntToDouble"),350,1100);Wire(Pin(E,TEXT("SizeX")),Pin(SX,TEXT("InInt")));auto SY=P.Math(TEXT("Conv_IntToDouble"),350,1350);Wire(Pin(E,TEXT("SizeY")),Pin(SY,TEXT("InInt")));
    auto Scale=P.Math(TEXT("FMin"),1000,1100);Wire(P.Op(TEXT("Divide_DoubleDouble"),Out(SX),nullptr,650,1100,TEXT("1280")),Pin(Scale,TEXT("A")));Wire(P.Op(TEXT("Divide_DoubleDouble"),Out(SY),nullptr,650,1350,TEXT("720")),Pin(Scale,TEXT("B")));
    FDraw D{P,Out(Scale)};auto Seq=P.Sequence(Then(Valid),1150,0,6);
    auto Hunter=P.Get(TEXT("bHunter"),1350,900,CC(),Ch);auto Alive=P.Not(P.Get(TEXT("bOut"),1350,1200,CC(),Ch),1650,1200);
    auto RoleColor=P.Choose(Hunter,Color(P,TEXT("(R=1,G=0.3,B=0.18,A=1)"),1350,1500),Color(P,TEXT("(R=0.2,G=0.9,B=0.85,A=1)"),1350,1750),1750,1500);
    D.X=2200;auto Exec=D.Rect(Seq->GetThenPinGivenIndex(0),24,24,425,205,TEXT("(R=0.012,G=0.022,B=0.03,A=0.91)"));Exec=D.Rect(Exec,24,24,5,205,TEXT(""),RoleColor);
    Exec=D.Text(Exec,P.Str(TEXT("ПОГОНЯ  /  СЕКТОР 04"),2400,750),44,39,1.05,TEXT("(R=0.75,G=0.8,B=0.85,A=1)"));
    auto Role=P.Choose(Hunter,P.Str(TEXT("ВЫ — ГОНЩИК"),2400,1100),P.Str(TEXT("ВЫ — БЕГЛЕЦ"),2400,1350),2800,1100);
    auto Started=P.Get(TEXT("bStarted"),1350,2050,SC(),S);
    Exec=D.Text(Exec,P.Choose(Started,Role,P.Str(TEXT("ОЖИДАНИЕ РОЛИ"),2400,1550),2800,1550),44,70,1.5,TEXT(""),RoleColor);
    auto HunterTask=P.Choose(P.Get(TEXT("bStick"),3100,1100,CC(),Ch),P.Str(TEXT("Палка найдена. Догоните беглецов"),3100,1400),P.Str(TEXT("Найдите палку. Подбор — E"),3100,1650),3500,1350);
    auto RunnerTask=P.Choose(P.Get(TEXT("bKey"),3900,1100,CC(),Ch),P.Str(TEXT("Ключ у вас. Откройте выход"),3900,1400),P.Choose(P.Get(TEXT("bGateOpen"),3900,1750,SC(),S),P.Str(TEXT("Выход открыт. Бегите к воротам"),3900,2000),P.Str(TEXT("Найдите ключ от ворот"),3900,2250),4300,1850),4700,1350);
    Exec=D.Text(Exec,P.Choose(Started,P.Choose(Hunter,HunterTask,RunnerTask,5100,1350),P.Str(TEXT("Подготовьтесь: раунд скоро начнётся"),5100,1550),5500,1350),44,112,1.0);
    auto Lives=P.Get(TEXT("Lives"),5100,1800,CC(),Ch);
    auto LifeText=Join(P,Join(P,P.Str(TEXT("ЖИЗНИ  "),5400,1850),IntegerString(P,Lives,5400,2100),5750,1850),P.Str(TEXT(" / 3"),5750,2100),6100,1850);
    Exec=D.Text(Exec,LifeText,44,150,1.0);
    for(int i=0;i<3;i++){
        auto HasLife=P.Compare(TEXT("Greater_IntInt"),Lives,nullptr,6500+i*450,1800,*FString::FromInt(i));auto CCol=P.Choose(HasLife,Color(P,TEXT("(R=0.22,G=0.88,B=0.6,A=1)"),6500+i*450,2050),Color(P,TEXT("(R=0.22,G=0.25,B=0.28,A=1)"),6500+i*450,2300),6850+i*450,2050);
        Exec=D.Rect(Exec,44+i*124,187,112,14,TEXT(""),CCol);
    }
    D.X=2200;D.Y=2900;Exec=D.Rect(Seq->GetThenPinGivenIndex(1),1025,24,230,105,TEXT("(R=0.012,G=0.022,B=0.03,A=0.91)"));
    auto Left=P.Math(TEXT("FCeil"),1350,3400);Wire(P.Get(TEXT("Remaining"),1050,3400,SC(),S),Pin(Left,TEXT("A")));
    auto Mins=P.Op(TEXT("Divide_IntInt"),Out(Left),nullptr,1650,3400,TEXT("60"));auto Secs=P.Op(TEXT("Percent_IntInt"),Out(Left),nullptr,1650,3650,TEXT("60"));
    auto SecStr=IntegerString(P,Secs,2000,3650);auto Pad=P.Choose(P.Compare(TEXT("Less_IntInt"),Secs,nullptr,2000,3900,TEXT("10")),Join(P,P.Str(TEXT("0"),2000,4200),SecStr,2350,4000),SecStr,2700,3700);
    auto Clock=Join(P,Join(P,IntegerString(P,Mins,2000,3400),P.Str(TEXT(":"),2350,3400),2700,3400),Pad,3050,3400);
    Exec=D.Text(Exec,P.Choose(P.Get(TEXT("bStarted"),3100,4000,SC(),S),P.Str(TEXT("ДО КОНЦА РАУНДА"),3100,4300),P.Str(TEXT("ПОДГОТОВКА"),3100,4550),3500,4000),1045,39,.9);
    Exec=D.Text(Exec,Clock,1045,66,1.8);
    auto Esc=Join(P,P.Str(TEXT("СПАСЛИСЬ  "),3900,3600),IntegerString(P,P.Get(TEXT("Escaped"),3900,3900,SC(),S),4200,3900),4550,3600);D.Text(Exec,Esc,1127,82,.8);
    D.X=2200;D.Y=4900;Exec=D.Rect(Seq->GetThenPinGivenIndex(2),24,664,1232,36,TEXT("(R=0.012,G=0.022,B=0.03,A=0.9)"));D.Text(Exec,P.Str(TEXT("WASD — движение    Мышь — обзор    Shift — бег    Пробел — прыжок    E — действие    ЛКМ — удар    R — новый раунд"),1500,5300),40,674,.87);
    // Contextual interaction prompt, also built from ordinary Blueprint conditions.
    auto Can=P.Branch(Seq->GetThenPinGivenIndex(3),Alive,1500,5800);auto L=P.Loop(Then(Can),PC(),false,2200,5800);auto Item=P.Cast(Pin(L,TEXT("LoopBody")),Pin(L,TEXT("Array Element")),PC(),2550,5800);auto It=Item->GetCastResultPin();
    auto Dist=P.Call(AActor::StaticClass(),TEXT("GetDistanceTo"),2550,6200,Ch);Wire(It,Pin(Dist,TEXT("OtherActor")));auto Near=P.Compare(TEXT("LessEqual_DoubleDouble"),Out(Dist),P.Get(TEXT("PickupRange"),2550,6450,CC(),Ch),2900,6200);
    auto Key=P.Get(TEXT("bKey"),2900,6550,PC(),It);auto Match=P.Op(TEXT("NotEqual_BoolBool"),Hunter,Key,3250,6500);
    auto Show=P.Branch(Then(Item),P.And(Near,Match,3600,6250),3950,5800);D.X=4350;D.Y=5800;Exec=D.Rect(Then(Show),400,540,480,55,TEXT("(R=0.02,G=0.04,B=0.05,A=0.94)"));D.Text(Exec,P.Choose(Key,P.Str(TEXT("[ E ]  ПОДОБРАТЬ КЛЮЧ"),3950,6600),P.Str(TEXT("[ E ]  ПОДОБРАТЬ ПАЛКУ"),3950,6850),4350,6600),435,557,1.2);
    auto GD=P.Math(TEXT("Vector_Distance2D"),2550,7300);Wire(P.Position(2200,7300,Ch),Pin(GD,TEXT("V1")));Wire(P.Get(TEXT("ExitLocation"),2200,7550,CC(),Ch),Pin(GD,TEXT("V2")));
    auto AtGate=P.Branch(Pin(L,TEXT("Completed")),P.And(P.Not(Hunter,2900,7550),P.Compare(TEXT("LessEqual_DoubleDouble"),Out(GD),P.Get(TEXT("ExitRange"),2900,7800,CC(),Ch),3250,7300),3600,7300),3950,7100);
    auto GatePrompt=P.Choose(P.Get(TEXT("bGateOpen"),3950,7550,SC(),S),P.Str(TEXT("[ E ]  ПОКИНУТЬ СЕКТОР"),3950,7850),P.Choose(P.Get(TEXT("bKey"),3950,8100,CC(),Ch),P.Str(TEXT("[ E ]  ОТКРЫТЬ ВОРОТА И ВЫЙТИ"),3950,8400),P.Str(TEXT("ДЛЯ ВЫХОДА НУЖЕН КЛЮЧ"),3950,8650),4350,8100),4750,7700);
    D.X=5150;D.Y=7100;Exec=D.Rect(Then(AtGate),375,540,530,55,TEXT("(R=0.02,G=0.04,B=0.05,A=0.94)"));D.Text(Exec,GatePrompt,410,557,1.15);
    auto End=P.Branch(Seq->GetThenPinGivenIndex(4),P.Or(P.Get(TEXT("bFinished"),1300,9300,SC(),S),P.Get(TEXT("bOut"),1300,9550,CC(),Ch),1650,9300),2000,9000);
    D.X=2400;D.Y=9000;Exec=D.Rect(Then(End),305,282,670,150,TEXT("(R=0.012,G=0.022,B=0.03,A=0.96)"));
    auto Own=P.Choose(P.Get(TEXT("bEscaped"),2000,9900,CC(),Ch),P.Str(TEXT("ВЫ ВЫБРАЛИСЬ"),2000,10200),P.Str(TEXT("ВЫ ВЫБЫЛИ"),2000,10450),2400,9900);
    auto EndText=P.Choose(P.Get(TEXT("bFinished"),2800,9900,SC(),S),P.Get(TEXT("Result"),2800,10150,SC(),S),Own,3200,9900);
    Exec=D.Text(Exec,EndText,345,317,1.55,TEXT("(R=1,G=0.7,B=0.3,A=1)"));D.Text(Exec,P.Str(TEXT("Нажмите R, чтобы начать новый раунд"),3200,10450),345,373,1.05);
    P.Comment(TEXT("HUD — РОЛЬ И ТРИ ЖИЗНИ. Текст Draw Text читает bHunter и Lives персонажа. Три прямоугольника показывают оставшиеся жизни."),-100,-150,8500,2800);
    P.Comment(TEXT("ТАЙМЕР И СПАСШИЕСЯ — данные из реплицируемого GameState"),1000,2750,4500,2000);
    P.Comment(TEXT("ПОДСКАЗКИ — клавиши управления, предмет поблизости и выход"),1000,4750,5300,4100);
    P.Comment(TEXT("РЕЗУЛЬТАТ — победа, побег или выбывание; подсказка перезапуска"),1000,8850,4500,2000);
    auto ServerTime=P.Call(AGameStateBase::StaticClass(),TEXT("GetServerWorldTimeSeconds"),1500,11600,S);
    auto Since=P.Op(TEXT("Subtract_DoubleDouble"),Out(ServerTime),P.Get(TEXT("LastHit"),1500,11850,CC(),Ch),1850,11600);
    auto Flash=P.Branch(Seq->GetThenPinGivenIndex(5),P.Compare(TEXT("Less_DoubleDouble"),Since,nullptr,2200,11600,TEXT("0.4")),2550,11200);
    D.X=2900;D.Y=11200;D.Rect(Then(Flash),0,0,1280,720,TEXT("(R=0.8,G=0.03,B=0.02,A=0.13)"));
    P.Comment(TEXT("ПОПАДАНИЕ — краткая красная вспышка помогает заметить потерю жизни"),1000,11050,3000,1200);
}
}
