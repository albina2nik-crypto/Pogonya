from pathlib import Path
r = Path(__file__).resolve().parents[1]
old = (r/'Source/PogonyaGraphEditor/PogonyaGraphBuilder.cpp').read_text(encoding='utf-8-sig')
part = old[old.index('void MovementGraphs('):old.index('\n}\n\nbool UPogonyaGraphBuilder::BuildCharacterGraphs')]
for a,b in [('AChaseCharacter::StaticClass()', 'CC()'),('AChaseState::StaticClass()', 'SC()'),('AChasePickup::StaticClass()', 'PC()'),('TEXT("Hurt")','TEXT("GraphReceiveHit")'),('TEXT("OnRep_Kind")','TEXT("UpdateAppearance")')]:
    part = part.replace(a,b)
part = part.replace('auto Pos=P.Position(2700,1900);auto Gate=P.Call(CC(),TEXT("ExitPosition"),2700,2050);','auto Pos=P.Position(2700,1900);auto Gate=P.Get(TEXT("ExitLocation"),2700,2050);')
part = part.replace('Wire(Pin(Gate,TEXT("ReturnValue")),Pin(Dist,TEXT("V2")))','Wire(Gate,Pin(Dist,TEXT("V2")))')
part = part.replace('auto Line=P.Call(CC(),TEXT("HasStrikeLine"),5400,1500);Wire(Target,Pin(Line,TEXT("Target")));\n    auto Hit=P.Branch(Pin(Candidate,TEXT("then")),P.And(Facing,Pin(Line,TEXT("ReturnValue")),5900,1100),6200,0);', '''auto Front=P.Branch(Then(Candidate),Facing,5800,0);
    auto Line=P.Call(UKismetSystemLibrary::StaticClass(),TEXT("LineTraceSingle"),6100,0);
    Wire(Then(Front),Pin(Line,TEXT("execute")));Wire(P.Position(5800,1500),Pin(Line,TEXT("Start")));Wire(P.Position(5800,1700,Target),Pin(Line,TEXT("End")));
    Value(Pin(Line,TEXT("TraceChannel")),TEXT("TraceTypeQuery1"));Value(Pin(Line,TEXT("bIgnoreSelf")),TEXT("true"));
    auto Ignore=P.Node<UK2Node_MakeArray>(5800,1900);Ignore->AllocateDefaultPins();Wire(P.Self(5500,1900),Pin(Ignore,TEXT("[0]")));Wire(Pin(Ignore,TEXT("Array")),Pin(Line,TEXT("ActorsToIgnore")));
    auto Break=P.Call(UGameplayStatics::StaticClass(),TEXT("BreakHitResult"),6400,1000);Wire(Pin(Line,TEXT("OutHit")),Pin(Break,TEXT("Hit")));
    auto Same=P.Op(TEXT("EqualEqual_ObjectObject"),Pin(Break,TEXT("HitActor")),Target,6700,1200);
    auto Hit=P.Branch(Then(Line),P.Or(P.Not(Out(Line),6400,1500),Same,7000,1300),7300,0);''')
part = part.replace('TEXT("GraphReceiveHit"),6500,0,Target','TEXT("GraphReceiveHit"),7650,0,Target')
part = part.replace('auto State=P.State(', 'auto GameStateRef=P.State(').replace(',State)', ',GameStateRef)')
part = part.replace('auto Gate=P.Get(', 'auto GatePoint=P.Get(').replace('Wire(Gate,', 'Wire(GatePoint,')
part = part.replace('void MovementGraphs(', 'static void MovementGraphs(').replace('void CameraSprintGraphs(', 'static void CameraSprintGraphs(').replace('void JumpGraph(', 'static void JumpGraph(').replace('void InteractionGraph(', 'static void InteractionGraph(').replace('void AttackGraph(', 'static void AttackGraph(').replace('void DamageGraph(', 'static void DamageGraph(')
out = '#include "PureBlueprintTools.h"\nnamespace PureGame {\n'+part+'\nvoid BuildCharacterLogic(){MovementGraphs(Character);CameraSprintGraphs(Character);JumpGraph(Character);InteractionGraph(Character);AttackGraph(Character);DamageGraph(Character);}\n}\n'
(r/'Source/PogonyaGraphEditor/PureBlueprintCharacter.cpp').write_text(out,encoding='utf-8')
