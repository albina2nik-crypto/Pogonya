#pragma once
// Editor-only asset authoring. Generated gameplay calls engine nodes and Blueprint events.
#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "Kismet/KismetArrayLibrary.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_Self.h"
#include "K2Node_Select.h"
#include "K2Node_MakeArray.h"
#include "K2Node_InputAxisEvent.h"
#include "K2Node_InputAction.h"
#include "K2Node_ExecutionSequence.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"

namespace PureGame {
extern int Errors;
extern UBlueprint *Character, *State, *Pickup, *Mode, *Controller, *HUD, *Gate;
inline UClass* CC(){return Character->GeneratedClass;}
inline UClass* SC(){return State->GeneratedClass;}
inline UClass* PC(){return Pickup->GeneratedClass;}
inline UClass* MC(){return Mode->GeneratedClass;}
inline FEdGraphPinType Type(FName Category, UObject* Object=nullptr, bool Array=false){
    FEdGraphPinType T; T.PinCategory=Category;T.PinSubCategoryObject=Object;
    if(Category==UEdGraphSchema_K2::PC_Real)T.PinSubCategory=UEdGraphSchema_K2::PC_Float;
    if(Array)T.ContainerType=EPinContainerType::Array;return T;
}
inline UEdGraphPin* Pin(UEdGraphNode* N,const TCHAR* Name){
    if(!N){++Errors;return nullptr;}auto P=N->FindPin(Name);
    if(!P){FString Names;for(auto Q:N->Pins)Names+=Q->PinName.ToString()+TEXT(", ");UE_LOG(LogTemp,Error,TEXT("PURE missing %s on %s/%s: %s"),Name,*N->GetGraph()->GetName(),*N->GetName(),*Names);++Errors;}return P;
}
inline void Wire(UEdGraphPin* A,UEdGraphPin* B){
    if(!A||!B){++Errors;return;}
    if(!GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A,B)){UE_LOG(LogTemp,Error,TEXT("PURE link %s.%s -> %s.%s"),*A->GetOwningNode()->GetName(),*A->PinName.ToString(),*B->GetOwningNode()->GetName(),*B->PinName.ToString());++Errors;}
}
inline void Value(UEdGraphPin* P,const FString& V){if(P)GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*P,V);else ++Errors;}
inline UEdGraphPin* Out(UEdGraphNode* N){return Pin(N,TEXT("ReturnValue"));}
inline UEdGraphPin* Then(UEdGraphNode* N){return Pin(N,TEXT("then"));}

struct FPage {
    UBlueprint* BP; UEdGraph* Graph;
    FPage(UBlueprint* B,const TCHAR* Name):BP(B),Graph(nullptr){
        for(auto G:BP->UbergraphPages)if(G->GetFName()==FName(Name))Graph=G;
        if(!Graph){Graph=FBlueprintEditorUtils::CreateNewGraph(BP,Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());FBlueprintEditorUtils::AddUbergraphPage(BP,Graph);}
    }
    template<class T>T* Node(int X,int Y){auto N=NewObject<T>(Graph);Graph->AddNode(N,false,false);N->CreateNewGuid();N->NodePosX=X;N->NodePosY=Y;return N;}
    UK2Node_CustomEvent* Event(const TCHAR* Name,int Y=0){
        for(auto N:Graph->Nodes)if(auto E=::Cast<UK2Node_CustomEvent>(N))if(E->CustomFunctionName==FName(Name)){E->NodePosY=Y;return E;}
        auto E=Node<UK2Node_CustomEvent>(0,Y);E->CustomFunctionName=Name;E->AllocateDefaultPins();return E;
    }
    UK2Node_Event* NativeEvent(UClass* Owner,const TCHAR* Name,int Y=0){auto E=Node<UK2Node_Event>(0,Y);E->EventReference.SetExternalMember(Name,Owner);E->bOverrideFunction=true;E->AllocateDefaultPins();return E;}
    UK2Node_CallFunction* Call(UClass* Owner,const TCHAR* Name,int X,int Y,UEdGraphPin* Target=nullptr){
        UK2Node_CallFunction* N=Owner==UKismetArrayLibrary::StaticClass()?Node<UK2Node_CallArrayFunction>(X,Y):Node<UK2Node_CallFunction>(X,Y);auto F=Owner?Owner->FindFunctionByName(Name):nullptr;
        if(!F){UE_LOG(LogTemp,Error,TEXT("PURE missing function %s.%s"),Owner?*Owner->GetName():TEXT("NULL"),Name);++Errors;return N;}
        N->SetFromFunction(F);N->AllocateDefaultPins();if(Target)Wire(Target,Pin(N,TEXT("self")));return N;
    }
    UEdGraphPin* Do(UEdGraphPin* Exec,UClass* Owner,const TCHAR* Name,int X,int Y,UEdGraphPin* Target=nullptr){auto N=Call(Owner,Name,X,Y,Target);Wire(Exec,Pin(N,TEXT("execute")));return Then(N);}
    UK2Node_CallFunction* Math(const TCHAR* Name,int X,int Y){return Call(UKismetMathLibrary::StaticClass(),Name,X,Y);}
    UEdGraphPin* Self(int X,int Y){auto N=Node<UK2Node_Self>(X,Y);N->AllocateDefaultPins();return Pin(N,TEXT("self"));}
    UEdGraphPin* Get(const TCHAR* Name,int X,int Y,UClass* Owner=nullptr,UEdGraphPin* Target=nullptr){
        auto N=Node<UK2Node_VariableGet>(X,Y);if(Target)N->VariableReference.SetExternalMember(Name,Owner);else N->VariableReference.SetSelfMember(Name);N->AllocateDefaultPins();if(Target)Wire(Target,Pin(N,TEXT("self")));return Pin(N,Name);
    }
    UK2Node_VariableSet* Set(const TCHAR* Name,int X,int Y,UClass* Owner=nullptr,UEdGraphPin* Target=nullptr){
        auto N=Node<UK2Node_VariableSet>(X,Y);if(Target)N->VariableReference.SetExternalMember(Name,Owner);else N->VariableReference.SetSelfMember(Name);N->AllocateDefaultPins();if(Target)Wire(Target,Pin(N,TEXT("self")));return N;
    }
    UEdGraphPin* Assign(UEdGraphPin* E,const TCHAR* Name,const TCHAR* V,int X,int Y,UClass* O=nullptr,UEdGraphPin* T=nullptr){auto N=Set(Name,X,Y,O,T);Wire(E,Pin(N,TEXT("execute")));Value(Pin(N,Name),V);return Then(N);}
    UEdGraphPin* AssignPin(UEdGraphPin* E,const TCHAR* Name,UEdGraphPin* V,int X,int Y,UClass* O=nullptr,UEdGraphPin* T=nullptr){auto N=Set(Name,X,Y,O,T);Wire(E,Pin(N,TEXT("execute")));Wire(V,Pin(N,Name));return Then(N);}
    UK2Node_IfThenElse* Branch(UEdGraphPin* E,UEdGraphPin* C,int X,int Y){auto N=Node<UK2Node_IfThenElse>(X,Y);N->AllocateDefaultPins();Wire(E,Pin(N,TEXT("execute")));Wire(C,Pin(N,TEXT("Condition")));return N;}
    UEdGraphPin* Not(UEdGraphPin* A,int X,int Y){auto N=Math(TEXT("Not_PreBool"),X,Y);Wire(A,Pin(N,TEXT("A")));return Out(N);}
    UEdGraphPin* Op(const TCHAR* Name,UEdGraphPin* A,UEdGraphPin* B,int X,int Y,const TCHAR* Literal=nullptr){auto N=Math(Name,X,Y);if(A)Wire(A,Pin(N,TEXT("A")));if(B)Wire(B,Pin(N,TEXT("B")));else if(Literal)Value(Pin(N,TEXT("B")),Literal);return Out(N);}
    UEdGraphPin* And(UEdGraphPin* A,UEdGraphPin* B,int X,int Y){return Op(TEXT("BooleanAND"),A,B,X,Y);}
    UEdGraphPin* Or(UEdGraphPin* A,UEdGraphPin* B,int X,int Y){return Op(TEXT("BooleanOR"),A,B,X,Y);}
    UEdGraphPin* Compare(const TCHAR* Name,UEdGraphPin* A,UEdGraphPin* B,int X,int Y,const TCHAR* Literal=nullptr){return Op(Name,A,B,X,Y,Literal);}
    UEdGraphPin* Choose(UEdGraphPin* Flag,UEdGraphPin* A,UEdGraphPin* B,int X,int Y){auto N=Node<UK2Node_Select>(X,Y);N->AllocateDefaultPins();Wire(Flag,Pin(N,TEXT("Index")));N->PinConnectionListChanged(Pin(N,TEXT("Index")));TArray<UEdGraphPin*> Options;N->GetOptionPins(Options);if(Options.Num()==2){Wire(B,Options[0]);Wire(A,Options[1]);}else ++Errors;return Out(N);}
    UEdGraphPin* Literal(const FEdGraphPinType& T,const FString& V,int X,int Y){
        // A select with identical options is a standard editable Blueprint literal.
        auto N=Node<UK2Node_Select>(X,Y);N->AllocateDefaultPins();auto I=Pin(N,TEXT("Index"));I->PinType=Type(UEdGraphSchema_K2::PC_Int);N->PinTypeChanged(I);Value(Pin(N,TEXT("Index")),TEXT("0"));Out(N)->PinType=T;N->PinTypeChanged(Out(N));TArray<UEdGraphPin*> Options;N->GetOptionPins(Options);for(auto P:Options)Value(P,V);return Out(N);
    }
    UEdGraphPin* Num(float V,int X,int Y){return Literal(Type(UEdGraphSchema_K2::PC_Real),FString::SanitizeFloat(V),X,Y);}
    UEdGraphPin* Str(const TCHAR* V,int X,int Y){return Literal(Type(UEdGraphSchema_K2::PC_String),V,X,Y);}
    UEdGraphPin* Obj(UObject* O,UClass* T,int X,int Y){auto N=Node<UK2Node_Select>(X,Y);N->AllocateDefaultPins();auto I=Pin(N,TEXT("Index"));I->PinType=Type(UEdGraphSchema_K2::PC_Int);N->PinTypeChanged(I);Value(Pin(N,TEXT("Index")),TEXT("0"));Out(N)->PinType=Type(UEdGraphSchema_K2::PC_Object,T);N->PinTypeChanged(Out(N));TArray<UEdGraphPin*> Opt;N->GetOptionPins(Opt);for(auto P:Opt)P->DefaultObject=O;return Out(N);}
    UEdGraphPin* Time(int X,int Y){return Out(Call(UGameplayStatics::StaticClass(),TEXT("GetTimeSeconds"),X,Y));}
    UEdGraphPin* CastPure(UEdGraphPin* Obj,UClass* T,int X,int Y){auto N=Node<UK2Node_DynamicCast>(X,Y);N->TargetType=T;N->SetPurity(true);N->AllocateDefaultPins();Wire(Obj,N->GetCastSourcePin());return N->GetCastResultPin();}
    UK2Node_DynamicCast* Cast(UEdGraphPin* E,UEdGraphPin* Obj,UClass* T,int X,int Y){auto N=Node<UK2Node_DynamicCast>(X,Y);N->TargetType=T;N->SetPurity(false);N->AllocateDefaultPins();Wire(E,Pin(N,TEXT("execute")));Wire(Obj,N->GetCastSourcePin());return N;}
    UEdGraphPin* StateRef(int X,int Y){return CastPure(Out(Call(UGameplayStatics::StaticClass(),TEXT("GetGameState"),X,Y)),SC(),X+260,Y);}
    UEdGraphPin* State(int X,int Y){return StateRef(X,Y);}
    UEdGraphPin* Valid(UEdGraphPin* V,int X,int Y){auto N=Call(UKismetSystemLibrary::StaticClass(),TEXT("IsValid"),X,Y);Wire(V,Pin(N,TEXT("Object")));return Out(N);}
    UEdGraphPin* Position(int X,int Y,UEdGraphPin* T=nullptr){return Out(Call(AActor::StaticClass(),TEXT("K2_GetActorLocation"),X,Y,T));}
    UEdGraphPin* Distance(UEdGraphPin* Other,int X,int Y){auto N=Call(AActor::StaticClass(),TEXT("GetDistanceTo"),X,Y);Wire(Other,Pin(N,TEXT("OtherActor")));return Out(N);}
    UEdGraphPin* Movement(int X,int Y){return Get(TEXT("CharacterMovement"),X,Y);}
    UEdGraphPin* Stop(UEdGraphPin* E,int X,int Y){return Do(E,UMovementComponent::StaticClass(),TEXT("StopMovementImmediately"),X,Y,Movement(X,Y+180));}
    UEdGraphPin* Refresh(UEdGraphPin* E,int X,int Y){return Do(E,CC(),TEXT("RefreshLook"),X,Y);}
    UEdGraphPin* Authority(UEdGraphPin* E,int X,int Y){return Then(Branch(E,Out(Call(AActor::StaticClass(),TEXT("HasAuthority"),X,Y+200)),X,Y));}
    UEdGraphPin* Phase(UEdGraphPin* E,int X,int Y){auto S=State(X-280,Y+550);auto B=Branch(E,Valid(S,X,Y+180),X+260,Y);auto Active=And(Get(TEXT("bStarted"),X+280,Y+180,SC(),S),Not(Get(TEXT("bFinished"),X+280,Y+350,SC(),S),X+510,Y+350),X+740,Y+180);return Then(Branch(Then(B),And(Active,Not(Get(TEXT("bOut"),X+510,Y+550),X+740,Y+550),X+970,Y+300),X+1200,Y));}
    UK2Node_MacroInstance* Macro(const TCHAR* Name,int X,int Y){auto M=LoadObject<UBlueprint>(nullptr,TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));UEdGraph* G=nullptr;for(auto Q:M->MacroGraphs)if(Q->GetFName()==FName(Name))G=Q;auto N=Node<UK2Node_MacroInstance>(X,Y);N->SetMacroGraph(G);N->AllocateDefaultPins();return N;}
    UK2Node_MacroInstance* Loop(UEdGraphPin* E,UClass* T,bool Breakable,int X,int Y){auto A=Call(UGameplayStatics::StaticClass(),TEXT("GetAllActorsOfClass"),X-300,Y);Pin(A,TEXT("ActorClass"))->DefaultObject=T;Wire(E,Pin(A,TEXT("execute")));auto N=Macro(Breakable?TEXT("ForEachLoopWithBreak"):TEXT("ForEachLoop"),X,Y);Wire(Pin(A,TEXT("OutActors")),Pin(N,TEXT("Array")));Wire(Then(A),Pin(N,TEXT("Exec")));return N;}
    UK2Node_ExecutionSequence* Sequence(UEdGraphPin* E,int X,int Y,int Count=2){auto N=Node<UK2Node_ExecutionSequence>(X,Y);N->AllocateDefaultPins();for(int i=2;i<Count;i++)N->AddInputPin();Wire(E,Pin(N,TEXT("execute")));return N;}
    UK2Node_SpawnActorFromClass* Spawn(UEdGraphPin* E,UClass* Class,UEdGraphPin* Location,int X,int Y){auto N=Node<UK2Node_SpawnActorFromClass>(X,Y);N->AllocateDefaultPins();Pin(N,TEXT("Class"))->DefaultObject=Class;N->PinDefaultValueChanged(Pin(N,TEXT("Class")));auto T=Math(TEXT("MakeTransform"),X-260,Y+180);if(Location)Wire(Location,Pin(T,TEXT("Location")));Wire(Out(T),Pin(N,TEXT("SpawnTransform")));Value(Pin(N,TEXT("CollisionHandlingOverride")),TEXT("AlwaysSpawn"));Wire(E,Pin(N,TEXT("execute")));return N;}
    void Comment(const TCHAR* T,int X,int Y,int W,int H){auto N=Node<UEdGraphNode_Comment>(X,Y);N->NodeComment=T;N->NodeWidth=W;N->NodeHeight=H;N->CommentColor=FLinearColor(.035f,.12f,.17f);N->FontSize=22;}
};
void BuildCharacterLogic();
void BuildCharacterVisuals();
void BuildBotLogic();
void BuildRoundLogic();
void BuildHUDLogic();
void BuildPickupGateLogic();
void LayoutBlueprint(UBlueprint* B);
}
