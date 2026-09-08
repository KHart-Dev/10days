#include "NumberUi.h"

#include <numbers>
#include <algorithm>

NumberUi::NumberUi()
	: Actor("plane.obj", "NumberUi") {}

void NumberUi::Initialize() {

	Actor::Initialize();

	DisableGravity();

	SetTexture("Textures/numbers/number.png");

	// 回転: X軸に90度
	auto& wt = GetWorldTransform();

	wt.scale = CalyxEngine::Vector3(0.5f, 0.9f, 1.0f);

	wt.eulerRotation.x =
		std::numbers::pi_v<float> *0.5f;

	wt.rotationSource = RotationSource::Euler;
	wt.inheritRotate = false;

	wt.Update();

	// 初期状態は完全表示
	SetColor({
		1.0f,
		1.0f,
		1.0f,
		1.0f
		});
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

		SetColor({
			1.0f,
			1.0f,
			1.0f,
			alpha
			});

		if (t >= 1.0f) {
			dead_ = true;
			Destroy();
			return;
		}
	}

	Actor::Update(dt);
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