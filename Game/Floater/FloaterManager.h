#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// std
#include <memory>
#include <vector>

class Floater;

CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Floater Manager", Icon = "UI/Tool/event.png")
class FloaterManager : public Actor {

public:

	FloaterManager();
	~FloaterManager() override;

	void Initialize() override;
	void Update(float dt) override;

	const std::vector<std::shared_ptr<Floater>>& GetFloaters() const { return floaters_; }
	float GetFieldHalfSize() const { return param_.spawnRadius; }

	/// <summary>1体を漂う側の管理から外して受け取る</summary>
	std::shared_ptr<Floater> Detach(const Floater* floater);

	void Reclaim(std::shared_ptr<Floater> floater);

	/// <summary>全部消してから湧かせ直す</summary>
	void Respawn();

	std::shared_ptr<Floater> CreateChained(const CalyxEngine::Vector3& pos);

private:

	void Spawn(int count);
	void Clear();

	std::vector<std::shared_ptr<Floater>> floaters_;


	struct FloaterManagerParam : CalyxEngine::SerializableObject {
		FloaterManagerParam() {
			AddField("spawnCount", spawnCount)
				.Category("Spawner")
				.Tooltip("湧き数");

			AddField("spawnRadius", spawnRadius)
				.Category("Spawner")
				.Tooltip("湧き範囲（フィールドの広さ）");

			AddField("driftSpeed", driftSpeed)
				.Category("Spawner")
				.Tooltip("漂う人の速さ");

			AddField("spinSpeed", spinSpeed)
				.Category("Spawner")
				.Tooltip("漂う人の回転速度");

			AddField("playBgmOnStart", playBgmOnStart)
				.Category("Spawner");
		}

		Guid ownerGuid_;
		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, ownerGuid_.ToString(), "Actor/Floater/FloaterManager" };
		}

		int spawnCount = 40;
		float spawnRadius = 30.0f;
		float driftSpeed = 1.2f;
		float spinSpeed = 1.2f;
		
		bool playBgmOnStart = true;
	};


	FloaterManagerParam param_;

	bool isSpawned_ = false;

public:

	void DerivativeGui() override;

};