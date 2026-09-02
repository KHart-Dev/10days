#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <memory>
#include <string>

CALYX_OBJECT(Category = GameObject, DisplayName = "NumberUi", Icon = "Textures/player/player.png")
class NumberUi : public Actor {

public:

	NumberUi();
	~NumberUi() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void SetNumber(int number) {
		std::string texturePath = "Textures/Numbers/" + std::to_string(number) + ".png";
		Actor::SetTexture(texturePath);
	}
	void SetPosition(const CalyxEngine::Vector3& pos) {
		auto& wt = GetWorldTransform();
		wt.translation = pos;
		wt.Update();
	}

	// 親オブジェクトを設定（Player 等）
	void SetParent(const std::shared_ptr<SceneObject>& parent, bool inheritScale = true) {
		SceneObject::SetParent(parent, inheritScale);
	}

private:

	void DisableGravity();

};

