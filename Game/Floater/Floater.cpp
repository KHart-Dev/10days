#include "Floater.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Random/Random.h>

// std
#include <numbers>

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
	spinRate_ = Random::Generate(-1.0f, 1.0f);
}

void Floater::Update(float dt) {

	Drift(dt);
	Actor::Update(dt);

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

}
