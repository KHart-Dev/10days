#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <memory>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

CALYX_OBJECT(Category = GameObject, DisplayName = "NumberUi", Icon = "Textures/player/player.png")
class NumberUi : public Actor {

public:

	NumberUi();
	~NumberUi() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void WtInitialize();

	// =========================
	// Scene保存 / Editor
	// =========================
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void DerivativeGui() override;

	// =========================
	// 表示数字
	// =========================
	void SetNumber(int number);

	int GetNumber() const {
		return number_;
	}

	void SetPosition(const CalyxEngine::Vector3& pos) {
		auto& wt = GetWorldTransform();
		wt.translation = pos;
		wt.Update();
	}

	// 親オブジェクトを設定
	void SetParent(
		const std::shared_ptr<SceneObject>& parent,
		bool inheritScale = true) {

		SceneObject::SetParent(parent, inheritScale);
	}

	void StartFade(float duration);

	bool IsDead() const {
		return dead_;
	}

private:

	void DisableGravity();

	// UVへ数字を反映
	void ApplyNumberUv();

private:

	// Editor / SetNumber 共通の現在の数字
	int number_ = 0;

	// SceneのJSONからnumberが読み込まれたか
	// Runtime生成されたNumberUiに勝手に0を適用しないために使用
	bool hasSerializedNumber_ = false;

	bool isFading_ = false;
	bool dead_ = false;

	float fadeTimer_ = 0.0f;
	float fadeDuration_ = 1.0f;
};