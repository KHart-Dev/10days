#include "SceneTransitionScene.h"

// engine
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// external
#include <externals/imgui/imgui.h>

SceneTransitionEvent::SceneTransitionEvent()
	: BaseEventObject("SceneTransitionEvent") {}

SceneTransitionEvent::~SceneTransitionEvent() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		Update
/////////////////////////////////////////////////////////////////////////////////////////
void SceneTransitionEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);
}

void SceneTransitionEvent::DerivativeGui() {
	BaseEventObject::DerivativeGui();
	ImGui::SeparatorText("Destination Scene");

	Guid droppedGuid = destinationSceneGuid_;
	if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Scene, &droppedGuid)) {
		destinationSceneGuid_ = droppedGuid;
	}

	const AssetRecord* record = destinationSceneGuid_.isValid()
		? AssetDatabase::GetInstance()->Get(destinationSceneGuid_)
		: nullptr;
	ImGui::TextWrapped("Current: %s",
		record && record->type == AssetType::Scene
			? record->sourcePath.filename().string().c_str()
			: "(none)");

	if(!destinationSceneGuid_.isValid()) ImGui::BeginDisabled();
	if(ImGui::Button("Transition To Scene")) {
		TransitionScene();
	}
	if(!destinationSceneGuid_.isValid()) ImGui::EndDisabled();
}

void SceneTransitionEvent::TransitionScene() {
	if(!destinationSceneGuid_.isValid()) return;
	SceneAPI::RequestSceneChange(destinationSceneGuid_);
}

void SceneTransitionEvent::ApplyDerivedConfigFromJson(
	[[maybe_unused]] const nlohmann::json& root,
	const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("destinationSceneGuid")) {
		destinationSceneGuid_ = derived->at("destinationSceneGuid").get<Guid>();
	}
}

void SceneTransitionEvent::ExtractDerivedConfigToJson(
	[[maybe_unused]] nlohmann::json& root,
	nlohmann::json& derived) const {
	derived["destinationSceneGuid"] = destinationSceneGuid_;
}
