#include "FloaterManager.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>

// game
#include "Floater.h"

// std
#include <algorithm>

FloaterManager::FloaterManager()
	:Actor("cone.obj", "FloaterManager") {}

void FloaterManager::Update([[maybe_unused]] float dt) {

	if (!isSpawned_) {
		Spawn(spawnCount_);
		isSpawned_ = true;
	}
}

void FloaterManager::Respawn() {

	Clear();
	Spawn(spawnCount_);
	isSpawned_ = true;
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

		floater->SetDriftSpeed(driftSpeed_);
		floater->SetSpinSpeed(spinSpeed_);
		floater->SetBounds(center, { spawnRadius_, 0.0f, spawnRadius_ });

		auto& wt = floater->GetWorldTransform();
		wt.translation = {
				center.x + Random::Generate(-spawnRadius_, spawnRadius_),
				center.y,
				center.z + Random::Generate(-spawnRadius_, spawnRadius_)
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
