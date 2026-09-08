#include "FloaterManager.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include <Engine/Foundation/Math/MathUtil.h>

// game
#include "Floater.h"
#include <Game/Audio/GameAudio.h>
#include <Game/Player/Player.h>

// std
#include <algorithm>
#include <cmath>

FloaterManager::FloaterManager()
	:Actor("cone.obj", "FloaterManager") {}

FloaterManager::~FloaterManager() {
	GameAudio::StopBgm();
}

void FloaterManager::Initialize() {
	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();
	SetDrawEnable(false);
}

void FloaterManager::Update([[maybe_unused]] float dt) {

	if (resultMode_) {
		GameAudio::PlayBgm(GameAudio::kBgmResult); // FloaterManagerがゲーム中しか存在しないとしとく（他シーンにも配置するならば別箇所へ）
		return;
	}

	if (!isSpawned_) {
		if (param_.playBgmOnStart) {
			GameAudio::PlayBgm(GameAudio::kBgmGame); // FloaterManagerがゲーム中しか存在しないとしとく（他シーンにも配置するならば別箇所へ）
			Spawn(param_.spawnCount);
		}
		isSpawned_ = true;
	}
}

void FloaterManager::Respawn() {

	Clear();
	Spawn(param_.spawnCount);
	isSpawned_ = true;
}

std::shared_ptr<Floater> FloaterManager::CreateChained(const CalyxEngine::Vector3& pos) {
	std::shared_ptr<Floater> floater = SceneAPI::InstantiatePrefabRoot<Floater>("Floater.prefab", pos);
	if (!floater) {
		return nullptr;
	}

	floater->Initialize();
	floater->SetDriftSpeed(param_.driftSpeed);
	floater->SetSpinSpeed(param_.spinSpeed);
	const CalyxEngine::Vector3 center = GetWorldTransform().translation;
	floater->SetBounds(center, param_.spawnRadius);
	floater->SetFieldLimits(fieldMinX_, fieldZLimit_);

	auto& wt = floater->GetWorldTransform();
	wt.Update();

	return floater;
}

void FloaterManager::Spawn(int count) {

	const CalyxEngine::Vector3 center = GetWorldTransform().translation;

	CalyxEngine::Vector3 keepOutCenter = center;
	float keepOut = 0.0f;
	if (auto* ctx = SceneContext::Current()) {
		if (auto player = ctx->FindFirst<Player>()) {
			keepOutCenter = player->GetWorldTransform().translation;
			keepOut = param_.spawnKeepOut * stageScale_;
			if (keepOut > 15.0f) {
				keepOut = 15.0f;
			}
		}
	}

	for (int i = 0; i < count; i++) {

		std::shared_ptr<Floater> floater = SceneAPI::InstantiatePrefabRoot<Floater>("Floater.prefab", center);
		if (!floater) {
			continue;
		}

		//floater->SetTransient(true);
		floater->Initialize();

		floater->SetDriftSpeed(param_.driftSpeed);
		floater->SetSpinSpeed(param_.spinSpeed);
		floater->SetBounds(center, param_.spawnRadius);
		floater->SetFieldLimits(fieldMinX_, fieldZLimit_);
		floater->SetStageScale(stageScale_);
		floater->ApplyStageScale();

		auto& wt = floater->GetWorldTransform();
		CalyxEngine::Vector3 pos{};
		for (int tries = 0; tries < 8; tries++) {
			const float angle = Random::Generate(0.0f, CalyxEngine::kTwoPi);
			const float radius = param_.spawnRadius * std::sqrtf(Random::Generate(0.0f, 1.0f));
			pos = { center.x + std::sinf(angle) * radius, center.y, center.z + std::cosf(angle) * radius };

			const float dx = pos.x - keepOutCenter.x;
			const float dz = pos.z - keepOutCenter.z;
			if ((dx * dx + dz * dz) >= keepOut * keepOut) {
				break;              // 十分離れた
			}
		}
		wt.translation = pos;
		wt.Update();

		// 座標を入れ終えてから。範囲外に湧いた個体はここで中心へ向く
		floater->BeginDrift();

		floaters_.push_back(std::move(floater));
	}
}

void FloaterManager::Clear() {

	for (const std::shared_ptr<Floater>& floater : floaters_) {
		if (floater) {
			floater->Destroy();
		}
	}
	floaters_.clear();
}

std::shared_ptr<Floater> FloaterManager::Detach(const Floater* floater) {

	auto it = std::find_if(floaters_.begin(), floaters_.end(),
		[floater](const std::shared_ptr<Floater>& sp) { return sp.get() == floater; });
	if (it == floaters_.end()) {
		return nullptr;
	}

	std::shared_ptr<Floater> detached = std::move(*it);
	floaters_.erase(it);
	return detached;
}

void FloaterManager::Reclaim(std::shared_ptr<Floater> floater) {
	if (!floater) {
		return;
	}
	floater->Unchain();
	floaters_.push_back(std::move(floater));
}

void FloaterManager::DerivativeGui() {
	param_.ShowGui();
}