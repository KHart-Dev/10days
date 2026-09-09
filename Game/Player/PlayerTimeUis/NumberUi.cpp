#include "NumberUi.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <numbers>
#include <algorithm>

NumberUi::NumberUi()
	: Actor("plane.obj", "NumberUi") {}

void NumberUi::Initialize() {

	Actor::Initialize();
	DisableGravity();
	SetTexture("Textures/numbers/number.png");

	SetColor({ 1.0f,1.0f,1.0f,1.0f });
	// ★ Sceneに保存されているNumberUiだけ適用
	if (hasSerializedNumber_) {
		ApplyNumberUv();
	}
}

void NumberUi::Update(float dt) {

	if (dead_) {
		Destroy();
		return;
	}

	if (isFading_) {

		fadeTimer_ += dt;

		const float duration =
			std::fmax(fadeDuration_, 0.001f);

		const float t =
			std::clamp(
				fadeTimer_ / duration,
				0.0f,
				1.0f
			);

		// 1 → 0
		const float alpha = 1.0f - t;

		SetColor({ 1.0f,1.0f,1.0f,alpha });

		if (t >= 1.0f) {
			dead_ = true;
			Destroy();
			return;
		}
	}

	Actor::Update(dt);
}

void NumberUi::WtInitialize() {
	auto& wt = GetWorldTransform();
	wt.scale = CalyxEngine::Vector3(0.5f, 0.9f, 1.0f);
	wt.eulerRotation.x = std::numbers::pi_v<float> *0.5f;
	wt.rotationSource = RotationSource::Euler;
	wt.inheritRotate = false;
	wt.Update();
}

void NumberUi::StartFade(float duration) {

	fadeDuration_ = std::fmax(duration, 0.001f);

	fadeTimer_ = 0.0f;
	isFading_ = true;
	dead_ = false;

	SetColor({
		1.0f,
		1.0f,
		1.0f,
		1.0f
		});
}

void NumberUi::DisableGravity() {

	auto& movement = GetCharacterMovement();

	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void NumberUi::SetNumber(int number) {

	number_ = std::clamp(number, 0, 9);
	ApplyNumberUv();
}

void NumberUi::ApplyNumberUv() {

	// Actor::Initialize前などの対策
	if (!model_) {
		return;
	}
	const CalyxEngine::Vector2 uvScale{ 0.1f,1.0f };
	const CalyxEngine::Vector2 uvOffset{ static_cast<float>(number_) * 0.1f,0.0f };
	SetUvScale(uvScale);
	model_->uvTransform.translate = uvOffset;
}

void NumberUi::ApplyConfigFromJson(const nlohmann::json& j) {

	Actor::ApplyConfigFromJson(j);
	const std::string typeKey(GetTypeName());
	const nlohmann::json* src = &j;
	if (j.contains(typeKey)) {
		src = &j.at(typeKey);
	}

	// 古いSceneデータにはnumberが存在しない可能性がある
	if (src->contains("number")) {
		number_ = std::clamp(src->value("number", number_), 0, 9);
		hasSerializedNumber_ = true;
		// Initialize後にApplyConfigされた場合
		if (model_) {
			ApplyNumberUv();
		}
	}
}

void NumberUi::ExtractConfigToJson(nlohmann::json& j) const {

	Actor::ExtractConfigToJson(j);
	const std::string typeKey(GetTypeName());
	nlohmann::json derived;
	derived["number"] = number_;
	j[typeKey] = std::move(derived);
}

void NumberUi::DerivativeGui() {

	if (ImGui::SliderInt("Number", &number_, 0, 9)) {
		ApplyNumberUv();
	}

	// Sceneに配置した直後、
	// まだSliderを操作していなくても現在値をプレビューする
	if (model_) {
		ApplyNumberUv();
	}
}