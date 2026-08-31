#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>

// std
#include <memory>
#include <vector>

class Floater;

CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Floater Manager", Icon = "UI/Tool/event.png")
class FloaterManager : public Actor {

public:

	FloaterManager();
	~FloaterManager() override = default;

	void Update(float dt) override;

	const std::vector<std::shared_ptr<Floater>>& GetFloaters() const { return floaters_; }

	/// <summary>1体を漂う側の管理から外して受け取る</summary>
	std::shared_ptr<Floater> Detach(const Floater* floater);

	/// <summary>全部消してから湧かせ直す</summary>
	void Respawn();

private:

	void Spawn(int count);
	void Clear();

	std::vector<std::shared_ptr<Floater>> floaters_;

	int spawnCount_ = 40;
	float spawnRadius_ = 30.0f;
	float driftSpeed_ = 1.2f;
	float spinSpeed_ = 1.2f;

	bool isSpawned_ = false;
};