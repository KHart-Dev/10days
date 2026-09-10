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

	void SetStageScale(float scale) { stageScale_ = scale; }

	/// <summary>Player の ClampToField と同じ壁を、湧かせる Floater へ配る</summary>
	void SetFieldLimits(float minX, float zLimit) {
		fieldMinX_ = minX;
		fieldZLimit_ = zLimit;
	}

	/// <summary>1体を漂う側の管理から外して受け取る</summary>
	std::shared_ptr<Floater> Detach(const Floater* floater);

	void Reclaim(std::shared_ptr<Floater> floater);

	/// <summary>全部消してから湧かせ直す</summary>
	void Respawn();

	std::shared_ptr<Floater> CreateChained(const CalyxEngine::Vector3& pos);

	void SetupResult() { resultMode_ = true; }

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
			AddField("spawnKeepOut", spawnKeepOut)
				.Category("Spawner");
		}

		Guid ownerGuid_;
		CalyxEngine::ParamPath GetParamPath() const override {
			// パス長対策: GUID全体だと <guid>/<guid>.json で78文字消費し、提出フォルダ名と合わせてMAX_PATHを超える
			return { CalyxEngine::ParamDomain::Game, ownerGuid_.ToString().substr(0, 8), "Actor/Floater/FloaterManager" };
		}

		int spawnCount = 40;
		float spawnRadius = 30.0f;
		float driftSpeed = 1.2f;
		float spinSpeed = 1.2f;
		float spawnKeepOut = 5.0f;
		
		bool playBgmOnStart = true;
	};


	FloaterManagerParam param_;

	bool isSpawned_ = false;
	bool resultMode_ = false;

	float stageScale_ = 1.0f;

	// Player::Initialize から入る。既定値は Player 側のパラメータ既定と揃えてある
	float fieldMinX_ = -20.0f;
	float fieldZLimit_ = 18.0f;

public:

	void DerivativeGui() override;

};