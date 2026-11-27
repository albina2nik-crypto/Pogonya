#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "PogonyaGameViewportClient.generated.h"

class SButton;
class SWidget;
class SWidgetSwitcher;

UCLASS()
class POGONYA_API UPogonyaGameViewportClient : public UGameViewportClient
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;

private:
    void ShowMainMenu(APlayerController& Controller);
    void StartPlaying();

    TWeakObjectPtr<UWorld> CheckedWorld;
    TWeakObjectPtr<APlayerController> MenuController;
    TSharedPtr<SWidget> MainMenu;
    TWeakPtr<SWidgetSwitcher> MenuPages;
    TWeakPtr<SButton> MenuPlayButton;
    TWeakPtr<SButton> MenuBackButton;
    bool bPreviousCursorVisibility = false;
};
