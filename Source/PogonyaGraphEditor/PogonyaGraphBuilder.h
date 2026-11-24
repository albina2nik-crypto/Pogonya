#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PogonyaGraphBuilder.generated.h"

UCLASS()
class UPogonyaGraphBuilder : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Pogonya Editor")
    static bool BuildCharacterGraphs();
    UFUNCTION(BlueprintCallable, Category="Pogonya Editor")
    static bool BuildBlueprintGame();
    UFUNCTION(BlueprintCallable, Category="Pogonya Editor")
    static bool OpenBlueprintGameGraphs();
};
