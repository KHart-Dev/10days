#include "Meteorite.h"

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

Meteorite::Meteorite()
	: Actor("plane.obj", "Meteorite") {

	// 生成直後から寝ているようにする。
	// プレハブに保存された回転があれば、そちらが後から勝つ
	worldTransform_.eulerRotation.x = kPitch;
	worldTransform_.rotationSource = RotationSource::Euler;
}

void Meteorite::Initialize() {
	Actor::Initialize();

	// プレハブの transform がコンストラクタの既定値を上書きするので入れ直す
	auto& wt = GetWorldTransform();
	wt.eulerRotation.x = kPitch;
	wt.rotationSource = RotationSource::Euler;

	DisableGravity();
}

void Meteorite::Update(float dt) {

	if (dead_) {
		return;
	}

	auto& wt = GetWorldTransform();
	wt.translation = wt.translation + velocity_ * dt;
	wt.Update();

	if (IsOutOfBounds()) {
		dead_ = true;
		Destroy();
		return;
	}

	Actor::Update(dt);
}

void Meteorite::Launch(const CalyxEngine::Vector3& velocity, float colliderRadius) {
	velocity_ = velocity;
	SetupCollider(colliderRadius);
}

void Meteorite::SetBounds(const CalyxEngine::Vector3& center, float radius) {
	boundsCenter_ = center;
	boundsRadius_ = radius;
}

bool Meteorite::IsOutOfBounds() const {

	// xz だけで見る。高さは判定に入れない
	const CalyxEngine::Vector3 pos = GetWorldTransform().translation;
	const float x = pos.x - boundsCenter_.x;
	const float z = pos.z - boundsCenter_.z;

	if ((x * x + z * z) <= (boundsRadius_ * boundsRadius_)) {
		return false;
	}

	// 中心から遠ざかっている（外向きの速度成分がある）ときだけ消す
	return (x * velocity_.x + z * velocity_.z) > 0.0f;
}

void Meteorite::SetupCollider(float radius) {
	InitializeCollider(ColliderKind::Sphere);

	if (Collider* collider = GetCollider()) {
		ColliderConfig config;
		if (const auto layerId = GameCollision::FindLayerId("Meteorite")) {
			config.layerId = *layerId;
		}
		config.radius = radius;
		// 押し返さず、当たったことだけ伝える
		config.isTrigger = true;
		collider->ApplyConfig(config);
	}

	if (collider_) {
		collider_->SetOwner(this);
	}
}

void Meteorite::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}
