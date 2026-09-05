#include "NumberUi.h"
#include <numbers>

NumberUi::NumberUi()
	: Actor("plane.obj", "NumberUi") {}

void NumberUi::Initialize() {
	Actor::Initialize();
	DisableGravity();
	SetTexture("Textures/numbers/number.png");

	// 回転: X軸に90度 (板を寝かせる)
	auto& wt = GetWorldTransform();
	wt.scale = CalyxEngine::Vector3(0.5f, 0.9f, 1.0f);
	wt.eulerRotation.x = std::numbers::pi_v<float> * 0.5f;
	wt.rotationSource = RotationSource::Euler;
	wt.inheritRotate = false;
	wt.Update();
}

void NumberUi::Update(float dt) {
	Actor::Update(dt);
}

void NumberUi::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}