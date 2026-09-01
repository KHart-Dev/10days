#include "Floater.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Math/MathUtil.h>

// game
#include "BodyNode.h"

// std
#include <algorithm>
#include <cmath>
#include <numbers>

Floater::Floater()
	: Actor("plane.obj", "Floater") {}

void Floater::Initialize() {
	Actor::Initialize();

	SetupCollider();
	DisableGravity();

	auto& wt = GetWorldTransform();
	wt.eulerRotation.x = std::numbers::pi_v<float> *0.5f; // pitch = 90deg
	wt.rotationSource = RotationSource::Euler;
	wt.eulerRotation.y = Random::Generate(0.0f, std::numbers::pi_v<float> *2.0f);
	wt.Update();

	driftDir_ = { Random::Generate(-1.0f, 1.0f), 0.0f, Random::Generate(-1.0f, 1.0f) };
	driftDir_ = driftDir_.Normalize();
	spinRate_ = Random::Generate(-1.0f, 1.0f);
	reachBias_ = Random::Generate(-1.0f, 1.0f);
}

void Floater::Update(float dt) {

	if (chained_) {
		// 連結中は自機の子として
	} else {
		Drift(dt);
	}
	Actor::Update(dt);

}

float Floater::GetYaw() const {
	const auto& wt = GetWorldTransform();
	float yaw = wt.eulerRotation.y;
	for (const BaseTransform* p = wt.parent; p; p = p->parent) {
		yaw += p->eulerRotation.y;
	}
	return yaw;
}

CalyxEngine::Vector3 Floater::GetArmWorld(int hand) const {
	return BodyNode::RotateY(BodyNode::kHand[hand], GetYaw());
}

CalyxEngine::Vector3 Floater::GetHandWorld(int hand) const {
	return GetWorldTransform().GetWorldPosition() + GetArmWorld(hand);
}

void Floater::MarkChained() {
	chained_ = true;
	driftDir_ = {};
	spinRate_ = 0.0f;
}

void Floater::ReachTowardArmAngle(int hand, float targetArmAngle, float step) {
	const float desiredYaw = targetArmAngle - BodyNode::HandAngle(hand);

	auto& wt = GetWorldTransform();
	wt.eulerRotation.y += std::clamp(BodyNode::WrapAngle(desiredYaw - wt.eulerRotation.y), -step, step);
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

	const float x = wt.translation.x - boundsCenter_.x;
	const float z = wt.translation.z - boundsCenter_.z;

	if ((x < -boundsHalf_.x && driftDir_.x < 0.0f) || (x > boundsHalf_.x && driftDir_.x > 0.0f)) {
		driftDir_.x = -driftDir_.x;
	}
	if ((z < -boundsHalf_.z && driftDir_.z < 0.0f) || (z > boundsHalf_.z && driftDir_.z > 0.0f)) {
		driftDir_.z = -driftDir_.z;
	}
}
