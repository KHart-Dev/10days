#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// std
#include <memory>
#include <vector>

class Meteorite;

/// <summary>ゲームシーンに置いて、流れ星をランダム間隔で湧かせるだけの役</summary>
CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Meteorite Spawner", Icon = "UI/Tool/event.png")
class MeteoriteSpawner : public Actor {

public:

	MeteoriteSpawner();
	~MeteoriteSpawner() override = default;

	void Initialize() override;
	void Update(float dt) override;

private:

	/// 1体湧かせる。湧く位置と向きはここで抽選
	void Spawn();

	/// Destroy 済みのものを一覧から外す
	void PruneDead();

	/// 次に湧くまでの秒数を引き直す
	float NextInterval() const;

	void DisableGravity();

	std::vector<std::shared_ptr<Meteorite>> meteorites_;

	float spawnTimer_ = 0.0f;

	struct MeteoriteSpawnerParam : CalyxEngine::SerializableObject {
		MeteoriteSpawnerParam() {
			AddField("intervalMin", intervalMin)
				.Category("Spawn")
				.Tooltip("湧く間隔の最小 (秒)");

			AddField("intervalMax", intervalMax)
				.Category("Spawn")
				.Tooltip("湧く間隔の最大 (秒)");

			AddField("maxAlive", maxAlive)
				.Category("Spawn")
				.Tooltip("同時に存在できる数");

			AddField("flowAngle", flowAngle)
				.Category("Area")
				.Tooltip("流れていく向き (deg)。0 で +Z、90 で +X。");

			AddField("spawnDistance", spawnDistance)
				.Category("Area")
				.Tooltip("流れの上流側、中心からどれだけ離れた所に湧くか");

			AddField("spawnSpread", spawnSpread)
				.Category("Area")
				.Tooltip("湧く帯の幅の半分。流れに直交する方向へこのぶん散らす");

			AddField("despawnRadius", despawnRadius)
				.Category("Area")
				.Tooltip("この円の外へ出て、かつ中心から遠ざかっていたら消える");

			AddField("spawnHeight", spawnHeight)
				.Category("Area")
				.Tooltip("湧く高さ。漂う人と同じ面を流すなら 0.5");

			AddField("speedMin", speedMin)
				.Category("Move")
				.Tooltip("速さの最小 (m/s)");

			AddField("speedMax", speedMax)
				.Category("Move")
				.Tooltip("速さの最大 (m/s)");

			AddField("directionSpread", directionSpread)
				.Category("Move")
				.Tooltip("flowAngle からのばらつき (deg)。0 で全部が平行に流れる");

			AddField("colliderRadius", colliderRadius)
				.Category("Collision")
				.Tooltip("当たり判定の半径");
		}

		Guid ownerGuid_;
		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, ownerGuid_.ToString(), "Actor/Meteorite/MeteoriteSpawner" };
		}

		float intervalMin = 1.0f;
		float intervalMax = 3.0f;
		int   maxAlive = 8;

		float flowAngle = 225.0f;
		float spawnDistance = 45.0f;
		float spawnSpread = 35.0f;
		float despawnRadius = 60.0f;
		float spawnHeight = 0.5f;

		float speedMin = 6.0f;
		float speedMax = 12.0f;
		float directionSpread = 25.0f;

		float colliderRadius = 1.0f;
	};

	MeteoriteSpawnerParam param_;

public:

	void DerivativeGui() override;

};
