#include "FallingMeteorite.h"

// engine
#include <Engine/Objects/Collider/SphereCollider.h>

// game
#include <Game/Collision/CollisionLayerUtil.h>

// std
#include <numbers>

namespace {
	// plane.obj は XY 平面に立っている。真上のカメラから見えるよう寝かせる
	constexpr float kPitch = std::numbers::pi_v<float> * 0.5f;
}

FallingMeteorite::FallingMeteorite()
	: Actor("plane.obj", "FallingMeteorite") {

	// 生成直後から寝ているようにする。
	// プレハブに保存された回転があれば、そちらが後から勝つ
	worldTransform_.eulerRotation.x = kPitch;
	worldTransform_.rotationSource = RotationSource::Euler;
}

void FallingMeteorite::Initialize() {
	Actor::Initialize();

	// プレハブの transform がコンストラクタの既定値を上書きするので入れ直す
	auto& wt = GetWorldTransform();
	wt.eulerRotation.x = kPitch;
	wt.rotationSource = RotationSource::Euler;

	DisableGravity();
}

void FallingMeteorite::Update(float dt) {

	switch (phase_) {
	case Phase::Falling:
	{
		auto& wt = GetWorldTransform();
		wt.translation.y -= fallSpeed_ * dt;

		if (wt.translation.y <= impactPos_.y) {
			wt.translation = impactPos_;
			EnableCollider(true);
			timer_ = 0.0f;
			phase_ = Phase::Impact;
		}
		wt.Update();
		break;
	}
	case Phase::Impact:
		timer_ += dt;
		if (timer_ >= impactHold_) {
			EnableCollider(false);
			phase_ = Phase::Done;
		}
		break;

	case Phase::Idle:
	case Phase::Done:
	default:
		break;
	}

	Actor::Update(dt);
}

void FallingMeteorite::Fall(const CalyxEngine::Vector3& impactPos,
							float fallHeight, float speed,
							float colliderRadius, float impactHold) {

	impactPos_ = impactPos;
	fallSpeed_ = speed;
	impactHold_ = impactHold;
	timer_ = 0.0f;

	SetupCollider(colliderRadius);
	EnableCollider(false);

	auto& wt = GetWorldTransform();
	wt.translation = { impactPos.x, impactPos.y + fallHeight, impactPos.z };
	wt.Update();

	phase_ = Phase::Falling;
}

void FallingMeteorite::SetupCollider(float radius) {
	InitializeCollider(ColliderKind::Sphere);

	if (Collider* collider = GetCollider()) {
		ColliderConfig config;
		if (const auto layerId = GameCollision::FindLayerId("Meteorite")) {
			config.layerId = *layerId;
		}
		config.radius = radius;
		config.isTrigger = true;
		config.isCollisionEnabled = false;
		collider->ApplyConfig(config);
	}

	if (collider_) {
		collider_->SetOwner(this);
	}
}

void FallingMeteorite::EnableCollider(bool enable) {

	Collider* collider = GetCollider();
	if (!collider) {
		return;
	}

	ColliderConfig config = collider->ExtractConfig();
	config.isCollisionEnabled = enable;
	collider->ApplyConfig(config);
}

void FallingMeteorite::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}
