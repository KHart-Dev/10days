#include "SceneFlow.h"

#include <Game/Result/ResultCarry.h>

#include <CalyxEngine/Project.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <algorithm>

namespace {
	void RequestScene(const char* relativePath) {
		SceneAPI::RequestSceneChange(Calyx::ResolveAssetPath(relativePath));
	}
}

void SceneFlow::GoToTitle() {
	ResultCarry::Clear();
	RequestScene(kTitleScenePath);
}

void SceneFlow::GoToAnimation() {
	RequestScene(kAnimationScenePath);
}

void SceneFlow::GoToAnimationFinal() {
	RequestScene(kAnimationFinalScenePath);
}

void SceneFlow::StartStage(int stageIndex) {
	ResultCarry::chain.clear();
	ResultCarry::stageIndex = (std::max)(0, stageIndex);
	//RequestScene(kGameScenePath);
	const std::string scenePath =kGameScenePath +std::to_string(stageIndex) +kSceneSuffix;
	RequestScene(scenePath.c_str());
}

void SceneFlow::GoToResult() {
	RequestScene(kResultScenePath);
}
