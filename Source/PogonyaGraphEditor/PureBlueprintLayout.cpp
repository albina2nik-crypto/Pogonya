#include "PureBlueprintTools.h"

namespace PureGame {
// Keep every editable node visible; allocate space for pin defaults and comments.
static FVector2D NodeSize(UEdGraphNode* N){
    int Inputs=0,Outputs=0,Longest=N->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Len();
    for(auto P:N->Pins)if(!P->bHidden){if(P->Direction==EGPD_Input)++Inputs;else ++Outputs;Longest=FMath::Max(Longest,P->PinName.ToString().Len()+FMath::Min(P->DefaultValue.Len(),65));}
    return FVector2D(FMath::Clamp(120+Longest*8,300,900),100+FMath::Max(Inputs,Outputs)*34);
}
void LayoutBlueprint(UBlueprint* B){
    B->UbergraphPages.Sort([](const UEdGraph& A,const UEdGraph& C){return A.GetName()<C.GetName();});
    TArray<UEdGraph*> Empty;
    for(auto G:B->UbergraphPages){
        if(G->GetFName()==TEXT("EventGraph")){
            bool Connected=false;for(auto N:G->Nodes)for(auto P:N->Pins)Connected|=P->LinkedTo.Num()>0;
            if(!Connected){Empty.Add(G);continue;}
        }
        TArray<UEdGraphNode_Comment*> Boxes;TArray<UEdGraphNode*> Nodes;
        for(auto N:G->Nodes)if(auto C=Cast<UEdGraphNode_Comment>(N)){
            const TPair<const TCHAR*,const TCHAR*> Titles[]={
                {TEXT("MOVE FORWARD"),TEXT("ДВИЖЕНИЕ ВПЕРЁД И НАЗАД — направление камеры; выбывший игрок двигаться не может")},
                {TEXT("MOVE RIGHT"),TEXT("ДВИЖЕНИЕ ВЛЕВО И ВПРАВО — направление камеры; выбывший игрок двигаться не может")},
                {TEXT("CAMERA"),TEXT("КАМЕРА — обзор мышью по горизонтали и вертикали")},
                {TEXT("SPRINT"),TEXT("БЕГ — Shift выбирает скорость по роли; отпускание возвращает ходьбу")},
                {TEXT("JUMP"),TEXT("ПРЫЖОК — нажатие и отпускание пробела; после выбывания недоступен")},
                {TEXT("E / SERVER"),TEXT("E / СЕРВЕР — проверить начало раунда и живого игрока; найти предмет поблизости")},
                {TEXT("PICKUP RULES"),TEXT("ПОДБОР — гонщик берёт палку, беглец берёт ключ; подобранный предмет исчезает с карты")},
                {TEXT("EXIT"),TEXT("ВЫХОД — ключ открывает общие ворота; каждый беглец нажимает E для побега")},
                {TEXT("ATTACK / SERVER"),TEXT("АТАКА / СЕРВЕР — активный раунд, роль гонщика, наличие палки и задержка между взмахами")},
                {TEXT("HIT VALIDATION"),TEXT("ПОПАДАНИЕ — живой беглец впереди, рядом и без стены между игроками; вызвать GraphReceiveHit")},
                {TEXT("RECEIVE HIT"),TEXT("ПОЛУЧЕНИЕ УРОНА — проверить неуязвимость; Lives минус 1; значение не ниже нуля")},
                {TEXT("THIRD HIT"),TEXT("ТРЕТЬЕ ПОПАДАНИЕ — выбывание, остановка движения, выпадение ключа и изменение вида")}
            };
            for(auto Pair:Titles)if(C->NodeComment.StartsWith(Pair.Key)){C->NodeComment=Pair.Value;break;}
            Boxes.Add(C);
        }else Nodes.Add(N);
        Boxes.Sort([](const UEdGraphNode_Comment& A,const UEdGraphNode_Comment& C){return A.NodePosY==C.NodePosY?A.NodePosX<C.NodePosX:A.NodePosY<C.NodePosY;});
        TMap<UEdGraphNode*,int> Groups;
        for(auto N:Nodes){int Best=Boxes.Num();int64 Area=MAX_int64;for(int i=0;i<Boxes.Num();i++){auto C=Boxes[i];if(N->NodePosX>=C->NodePosX&&N->NodePosX<C->NodePosX+C->NodeWidth&&N->NodePosY>=C->NodePosY&&N->NodePosY<C->NodePosY+C->NodeHeight){int64 Size=int64(C->NodeWidth)*C->NodeHeight;if(Size<Area){Best=i;Area=Size;}}}Groups.Add(N,Best);}
        int Top=0;
        for(int Group=0;Group<=Boxes.Num();Group++){
            TArray<UEdGraphNode*> Section;for(auto N:Nodes)if(Groups[N]==Group)Section.Add(N);if(Section.IsEmpty())continue;
            Section.Sort([](const UEdGraphNode& A,const UEdGraphNode& C){return A.NodePosY==C.NodePosY?A.NodePosX<C.NodePosX:A.NodePosY<C.NodePosY;});
            int Left=MAX_int32,MinY=MAX_int32;for(auto N:Section){Left=FMath::Min(Left,N->NodePosX);MinY=FMath::Min(MinY,N->NodePosY);}
            TArray<FBox2D> Occupied;double Right=0,Bottom=Top;
            for(auto N:Section){auto Size=NodeSize(N);double X=(N->NodePosX-Left)*1.8,Y=Top+(N->NodePosY-MinY)*1.5;bool Overlap=true;
                while(Overlap){Overlap=false;FBox2D Rect(FVector2D(X,Y),FVector2D(X+Size.X+80,Y+Size.Y+100));for(auto R:Occupied)if(Rect.Intersect(R)){Y=R.Max.Y+60;Overlap=true;break;}}
                N->NodePosX=FMath::RoundToInt(X/16)*16;N->NodePosY=FMath::RoundToInt(Y/16)*16;
                Occupied.Add(FBox2D(FVector2D(X,Y),FVector2D(X+Size.X+80,Y+Size.Y+100)));Right=FMath::Max(Right,X+Size.X);Bottom=FMath::Max(Bottom,Y+Size.Y);
            }
            if(Boxes.IsValidIndex(Group)){auto C=Boxes[Group];C->NodePosX=-80;C->NodePosY=Top-130;C->NodeWidth=FMath::CeilToInt(Right+160);C->NodeHeight=FMath::CeilToInt(Bottom-Top+230);}
            Top=FMath::CeilToInt(Bottom+450);
        }
    }
    for(auto G:Empty)FBlueprintEditorUtils::RemoveGraph(B,G);
}
}
