#include "Meteorite.h"

// engine
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

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

Meteorite::~Meteorite() {

	StopEffect();
}

void Meteorite::Initialize() {
	Actor::Initialize();

	// プレハブの transform がコンストラクタの既定値を上書きするので入れ直す
	auto& wt = GetWorldTransform();
	wt.eulerRotation.x = kPitch;
	wt.rotationSource = RotationSource::Euler;

	moveEffect_.Load("Meteorite");

	DisableGravity();
}

void Meteorite::Update(float dt) {

	if (dead_) {
		return;
	}

	auto& wt = GetWorldTransform();
	wt.translation = wt.translation + velocity_ * dt;

	wt.eulerRotation.y += spinSpeed_ * dt;
	wt.rotationSource = RotationSource::Euler;
	wt.Update();

	if (moveHandle_.IsValid()) {
		CalyxEngine::Vector3 pos = GetWorldPosition();
		pos.y -= 0.25f;
		EffectAPI::Player()->SetTransform(
			moveHandle_,
			pos,
			CalyxEngine::Quaternion::MakeIdentity(),
			{ 1.0f, 1.0f, 1.0f });
	}

	if (IsOutOfBounds()) {
		dead_ = true;

		// 本体が消えても尾だけ残らないよう、Destroy より先に止める
		StopEffect();
		Destroy();
		return;
	}

	Actor::Update(dt);
}

void Meteorite::Launch(const CalyxEngine::Vector3& velocity, float colliderRadius, float spinSpeed) {
	velocity_ = velocity;
	spinSpeed_ = spinSpeed;
	SetupCollider(colliderRadius);
	moveHandle_ = EffectAPI::Play(moveEffect_, GetWorldPosition());
}

void Meteorite::SetBounds(const CalyxEngine::Vector3& center, float radius) {
	boundsCenter_ = center;
	boundsRadius_ = radius;
}

void Meteorite::StopEffect() {

	if (!moveHandle_.IsValid()) {
		return;
	}

	if (SceneContext::Current()) {
		EffectAPI::Stop(moveHandle_);
	}

	moveHandle_ = {};
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
		collider_->SetIsDrawCollider(false);
	}
}

void Meteorite::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}
