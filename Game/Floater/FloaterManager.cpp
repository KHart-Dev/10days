#include "FloaterManager.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include "Floater.h"
#include <Game/Audio/GameAudio.h>

// std
#include <algorithm>

FloaterManager::FloaterManager()
	:Actor("cone.obj", "FloaterManager") {}

FloaterManager::~FloaterManager() {
	GameAudio::StopBgm();
}

void FloaterManager::Initialize() {
	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();
}

void FloaterManager::Update([[maybe_unused]] float dt) {

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
	floater->SetBounds(center, { param_.spawnRadius, 0.0f, param_.spawnRadius });

	auto& wt = floater->GetWorldTransform();
	wt.Update();

	return floater;
}

void FloaterManager::Spawn(int count) {

	const CalyxEngine::Vector3 center = GetWorldTransform().translation;

	for (int i = 0; i < count; i++) {

		std::shared_ptr<Floater> floater = SceneAPI::InstantiatePrefabRoot<Floater>("Floater.prefab", center);
		if (!floater) {
			continue;
		}

		//floater->SetTransient(true);
		floater->Initialize();

		floater->SetDriftSpeed(param_.driftSpeed);
		floater->SetSpinSpeed(param_.spinSpeed);
		floater->SetBounds(center, { param_.spawnRadius, 0.0f, param_.spawnRadius });

		auto& wt = floater->GetWorldTransform();
		wt.translation = {
				center.x + Random::Generate(-param_.spawnRadius, param_.spawnRadius),
				center.y,
				center.z + Random::Generate(-param_.spawnRadius, param_.spawnRadius)
		};
		wt.Update();

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