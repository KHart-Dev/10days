#include "Floater.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Math/MathUtil.h>

// std
#include <numbers>


namespace {
	constexpr float kArmLength = 0.6f;
	constexpr float kArmSpread = std::numbers::pi_v<float>*0.5f;
}

Floater::Floater()
	: Actor("plane.obj", "Floater") {}

void Floater::Initialize() {
	Actor::Initialize();

	SetupCollider();
	DisableGravity();

	auto& wt = GetWorldTransform();
	// 常に X 軸に -90度回転させるため、回転ソースをオイラーにして固定ピッチを設定する
	wt.eulerRotation.x = std::numbers::pi_v<float> *0.5f; // pitch = -90deg
	wt.rotationSource = RotationSource::Euler;
	// 向きは湧いた時点でばらしておく
	wt.eulerRotation.y = Random::Generate(0.0f, std::numbers::pi_v<float> *2.0f);
	wt.Update();

	driftDir_ = { Random::Generate(-1.0f, 1.0f), 0.0f, Random::Generate(-1.0f, 1.0f) };
	driftDir_ = driftDir_.Normalize();
	spinRate_ = Random::Generate(-1.0f, 1.0f);
}

void Floater::Update(float dt) {

	if (chained_) {

	} else {
		Drift(dt);
	}
	Actor::Update(dt);

}

float Floater::GetYaw() const {
	return GetWorldTransform().eulerRotation.y;
}

CalyxEngine::Vector3 Floater::GetArmWorld(int hand) const {
	const float sign = (hand == 0) ? 1.0f : -1.0f;
	const float armAngle = GetYaw() + reachBias_ + kArmSpread * sign;

	const CalyxEngine::Matrix4x4 rot = CalyxEngine::MakeRotateYMatrix(armAngle);
	return CalyxEngine::TransformNormal({ 0.0f,0.0f,kArmLength }, rot);
}

CalyxEngine::Vector3 Floater::GetHandWorld(int hand) const {
	return GetWorldTransform().translation + GetArmWorld(hand);
}

void Floater::MarkChained() {
	chained_ = true;
	driftDir_ = {};
	spinRate_ = 0.0f;
}

void Floater::ReachTowardArmAngle(int hand, float targetArmAngle, float step) {
	const float sign = (hand == 0) ? 1.0f : -1.0f;
	const float desiredYaw = targetArmAngle - reachBias_ - kArmSpread * sign;

	auto& wt = GetWorldTransform();
	wt.eulerRotation.y = CalyxEngine::LerpShortAngle(wt.eulerRotation.y, desiredYaw, step);
	wt.Update();
}

void Floater::SetupCollider() {
	InitializeCollider(ColliderKind::Box);

	if (Collider* collider = GetCollider()) {
		collider->ApplyConfig(ColliderConfig{});
	}
}

void Floater::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void Floater::Drift(float dt) {
	auto& wt = GetWorldTransform();

	wt.translation = wt.translation + driftDir_ * (driftSpeed_ * dt);
	wt.translation.y = 0.5f;
	wt.eulerRotation.y += spinRate_ * spinSpeed_ * dt;

	BounceOnEdge();

	wt.Update();
}

void Floater::BounceOnEdge() {
	auto& wt = GetWorldTransform();

	CalyxEngine::Vector3 offset = wt.translation - boundsCenter_;
	offset.y = 0.0f;

	const float dist = offset.Length();
	if (dist < boundsRadius_ || dist <= 1e-4) {
		return;
	}

	const CalyxEngine::Vector3 normal = offset / dist;
	const float dot = CalyxEngine::Vector3::Dot(driftDir_, normal);
	if (dot > 0.0f) {
		driftDir_ = driftDir_ - normal * (2.0f * dot);
	}

	wt.translation = boundsCenter_ + normal * boundsRadius_;
	wt.translation.y = 0.5f;
}
