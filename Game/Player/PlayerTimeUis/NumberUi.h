#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <memory>
#include <string>
#include <algorithm>

CALYX_OBJECT(Category = GameObject, DisplayName = "NumberUi", Icon = "Textures/player/player.png")
class NumberUi : public Actor {

public:

	NumberUi();
	~NumberUi() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void SetNumber(int number) {
		int currentNumber = std::clamp(number, 0, 9);

		const CalyxEngine::Vector2 uvScale{
			0.1f,
			1.0f
		};

		const CalyxEngine::Vector2 uvOffset{
			static_cast<float>(currentNumber) * 0.1f,
			0.0f
		};

		SetUvScale(uvScale);
		model_->uvTransform.translate = uvOffset;
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

