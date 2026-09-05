#include "MeteoriteSpawner.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include "Meteorite.h"

// std
#include <algorithm>
#include <cmath>

namespace {
	// Resources/Assets/Prefabs/ からの相対パス
	constexpr const char* kMeteoritePrefab = "Meteorite.prefab";
}

MeteoriteSpawner::MeteoriteSpawner()
	: Actor("cone.obj", "MeteoriteSpawner") {}

void MeteoriteSpawner::Initialize() {
	Actor::Initialize();

	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();

	DisableGravity();
	SetDrawEnable(false);

	spawnTimer_ = NextInterval();
}

void MeteoriteSpawner::Update(float dt) {

	PruneDead();

	spawnTimer_ -= dt;
	if (spawnTimer_ <= 0.0f) {
		Spawn();
		spawnTimer_ = NextInterval();
	}

	Actor::Update(dt);
}

void MeteoriteSpawner::Spawn() {

	if (static_cast<int>(meteorites_.size()) >= param_.maxAlive) {
		return;
	}

	const CalyxEngine::Vector3 center = GetWorldTransform().translation;

	// 流れていく向き
	const float flowYaw = CalyxEngine::ToRadians(param_.flowAngle);
	const CalyxEngine::Vector3 flow{ std::sinf(flowYaw), 0.0f, std::cosf(flowYaw) };

	// 流れに直交する、湧く位置はこの線上
	const CalyxEngine::Vector3 side{ flow.z, 0.0f, -flow.x };
	const float lateral = Random::Generate(-param_.spawnSpread, param_.spawnSpread);

	const CalyxEngine::Vector3 pos{
		center.x - flow.x * param_.spawnDistance + side.x * lateral,
		center.y + param_.spawnHeight,
		center.z - flow.z * param_.spawnDistance + side.z * lateral
	};

	const float heading = flowYaw + CalyxEngine::ToRadians(
		Random::Generate(-param_.directionSpread, param_.directionSpread));

	const CalyxEngine::Vector3 direction{ std::sinf(heading), 0.0f, std::cosf(heading) };

	const float speedLow = (param_.speedMin < param_.speedMax) ? param_.speedMin : param_.speedMax;
	const float speedHigh = (param_.speedMin < param_.speedMax) ? param_.speedMax : param_.speedMin;
	const float speed = Random::Generate(speedLow, speedHigh);

	std::shared_ptr<Meteorite> meteorite =
		SceneAPI::InstantiatePrefabRoot<Meteorite>(kMeteoritePrefab, pos);
	if (!meteorite) {
		return;
	}
	meteorite->Initialize();

	auto& wt = meteorite->GetWorldTransform();
	wt.translation = pos;
	wt.Update();

	// 回る速さは deg/s で持つ。向きは1体ごとに半々で振り分ける
	const float spinLow = (param_.spinSpeedMin < param_.spinSpeedMax) ? param_.spinSpeedMin : param_.spinSpeedMax;
	const float spinHigh = (param_.spinSpeedMin < param_.spinSpeedMax) ? param_.spinSpeedMax : param_.spinSpeedMin;

	float spin = CalyxEngine::ToRadians(Random::Generate(spinLow, spinHigh));
	if (Random::Generate(0.0f, 1.0f) < 0.5f) {
		spin = -spin;
	}

	meteorite->SetBounds(center, param_.despawnRadius);
	meteorite->Launch(direction * speed, param_.colliderRadius, spin);

	meteorites_.push_back(std::move(meteorite));
}

void MeteoriteSpawner::PruneDead() {

	meteorites_.erase(
		std::remove_if(meteorites_.begin(), meteorites_.end(),
			[](const std::shared_ptr<Meteorite>& meteorite) {
				return !meteorite || meteorite->IsDead();
			}),
		meteorites_.end());
}

float MeteoriteSpawner::NextInterval() const {

	const float low = (param_.intervalMin < param_.intervalMax) ? param_.intervalMin : param_.intervalMax;
	const float high = (param_.intervalMin < param_.intervalMax) ? param_.intervalMax : param_.intervalMin;
	return Random::Generate(low, high);
}

void MeteoriteSpawner::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void MeteoriteSpawner::DerivativeGui() {
	ImGui::Text("Alive: %d / %d", static_cast<int>(meteorites_.size()), param_.maxAlive);
	ImGui::Text("Next : %.2f s", spawnTimer_);
	param_.ShowGui();
}
