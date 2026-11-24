#include "Modules/ModuleManager.h"
#include "BlueprintTestHarness.h"
class FPogonyaModule : public FDefaultGameModuleImpl {
public:
    virtual void StartupModule() override { StartBlueprintTestHarness(); }
    virtual void ShutdownModule() override { StopBlueprintTestHarness(); }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FPogonyaModule, Pogonya, "Pogonya");
