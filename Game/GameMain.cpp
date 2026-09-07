#include <CalyxEngine/CalyxEngine.h>
#include <CalyxEngine/Engine.h>

#include <Generated/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include "GameApplication.h"

extern "C" __declspec(dllexport) Calyx::Application* CreateCalyxApplication() {

	CalyxEngine::RegisterGeneratedGameSceneObjects();
	return new GameApplication();
}

extern "C" __declspec(dllexport) void DestroyCalyxApplication(Calyx::Application* application) {
	delete application;
}
