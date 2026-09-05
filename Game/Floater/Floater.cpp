#include "Floater.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Context/SceneContext.h>

// game
#include "BodyNode.h"
#include <Game/Audio/GameAudio.h>
#include <Game/Actor/Obstacle/Thorn.h>
#include <Game/Collision/CollisionLayerUtil.h>
#include <Game/Demo/3D/Actor/DemoCamera/DemoCameraPivot.h>
#include <Game/Meteorite/InGame/Meteorite.h>
#include <Game/Meteorite/InResult/FallingMeteorite.h>

// std
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	// 板を寝かせるための固定ピッチ
	constexpr float kPitch = std::numbers::pi_v<float> * 0.5f;
	constexpr float kBreakShakeDuration = 0.5f;
	constexpr float kBreakShakeIntensity = 15.0f;

	void RequestBreakCameraShake() {
		auto* context = SceneContext::Current();
		auto* library = context ? context->GetObjectLibrary() : nullptr;
		if(!library) {
			return;
		}

		auto pivots = library->FindByClassName("DemoCameraPivot");
		if(!pivots.empty()) {
			if(auto* pivot = dynamic_cast<DemoCameraPivot*>(pivots.front().get())) {
				pivot->RequestShake(kBreakShakeDuration, kBreakShakeIntensity);
			}
		}
	}
}

Floater::Floater()
	: Actor("plane.obj", "Floater") {}

void Floater::Initialize() {
	Actor::Initialize();

	SetupCollider();
	DisableGravity();

	auto& wt = GetWorldTransform();
	wt.eulerRotation.x = kPitch;
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

		if (breakedCooltime_ > 0.0f) {
			breakedCooltime_ -= dt;
		}
	}
	Actor::Update(dt);

}

void Floater::OnCollisionEnter(Collider* other) {
	BaseGameObject* owner = other ? other->GetOwner() : nullptr;
	if (!chained_ || !owner) {
		return;
	}

	auto* meteorite = dynamic_cast<Meteorite*>(owner);
	const bool hitObstacle = meteorite
		|| dynamic_cast<Thorn*>(owner)
		|| dynamic_cast<FallingMeteorite*>(owner);

	if (!hitObstacle) {
		return;
	}

	MarkBreak();
	RequestBreakCameraShake();

	if (meteorite) {
		meteorite->MarkDead();
	}
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

bool Floater::CanConnect() const {
	bool result = true;
	if (IsChained() || breakedCooltime_ > 0.0f) {
		result = false;
	}
	return result;
}

void Floater::SetChainedTransform(const CalyxEngine::Vector3& pos, float worldYaw) {
	auto& wt = GetWorldTransform();

	wt.translation = pos;
	// ピッチとロールを毎フレーム固定値で入れ直す。
	// 親子付けで親のヨーを合成すると、寝かせた軸に乗って板が転がるため
	wt.eulerRotation = { kPitch, worldYaw, 0.0f };
	wt.rotationSource = RotationSource::Euler;
	wt.Update();
}

void Floater::MarkChained() {
	ApplyChainedLook();
	GameAudio::PlaySe(GameAudio::kSeConnect);
}

void Floater::RestoreChained() {
	ApplyChainedLook();
}

void Floater::MarkBreak() {
	breakMark_ = true;
}

void Floater::Unchain() {
	chained_ = false;
	breakMark_ = false;
	breakedCooltime_ = 3.0f;
	driftDir_ = { Random::Generate(-1.0f, 1.0f), 0.0f, Random::Generate(-1.0f, 1.0f) }; // とりあえず今はランダム方向に壊れる
	driftDir_ = driftDir_.Normalize();
	spinRate_ = Random::Generate(-1.0f, 1.0f);
	GameAudio::PlaySe(GameAudio::kSeDamage);
	SetTexture("Textures/human/human.png");
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
		const char* layerName = "Player";
		const auto layerId = GameCollision::FindLayerId(layerName);

		ColliderConfig config;
		if (layerId) {
			config.layerId = *layerId;
		}
		collider->ApplyConfig(config);
		collider_->SetOwner(this);
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

void Floater::ApplyChainedLook() {
	chained_ = true;
	driftDir_ = {};
	spinRate_ = 0.0f;
	SetTexture("Textures/connectHuman/connectHuman.png");
}
