#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <memory>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

CALYX_OBJECT(Category = GameObject, DisplayName = "NumberUi", Icon = "Textures/player/player.png")
class TutorialUi : public Actor {

public:
	TutorialUi();
	~TutorialUi() override = default;
	void Initialize() override;
	void Update(float dt) override;

	void SetTexture(const std::string& textureName) {
		Actor::SetTexture(textureName);
	}

	void SetScale(const CalyxEngine::Vector3& scale) {
		auto& wt = GetWorldTransform();
		wt.scale = scale;
		wt.Update();
	}

	void SetPosition(const CalyxEngine::Vector3& pos) {
		auto& wt = GetWorldTransform();
		wt.translation = pos;
		wt.Update();
	}

	void SetRotation(const CalyxEngine::Quaternion& rotation) {
		auto& wt = GetWorldTransform();
		wt.rotation = rotation;
		wt.Update();
	}

	// 親オブジェクトを設定
	void SetParent(
		const std::shared_ptr<SceneObject>& parent,
		bool inheritScale = true) {

		SceneObject::SetParent(parent, inheritScale);
	}

	void SetAlpha(float alpha) {
		SetColor({ 1.0f,1.0f,1.0f,alpha });
	}

};

