#pragma once

// engine
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

/*-----------------------------------------------------------------------------------------
 * SceneTransitionScene
 * - シーン遷移イベント
 * - シーンを遷移させるイベント
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = Event, DisplayName = "Scene Transition")
class SceneTransitionEvent final : public BaseEventObject {
public:
	SceneTransitionEvent();
	~SceneTransitionEvent() override;

	void AlwaysUpdate(float dt) override;
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root,
									const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root,
									nlohmann::json& derived) const override;

	std::string_view GetObjectClassName() const override { return "SceneTransitionEvent"; }

private:
	void TransitionScene();

	Guid destinationSceneGuid_{};

};

