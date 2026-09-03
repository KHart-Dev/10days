#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// std
#include <memory>

class FallingMeteorite;

/// <summary>落とし方の設定。地点ごとに変えないものは Director がまとめて配る</summary>
struct MeteoriteFallSettings {
	int   blinkTimes = 3;			//< 何回点滅させてから落とすか
	float blinkPeriod = 0.4f;		//< 点滅1回ぶんの秒数。落ちるまでの時間はこの2つで決まる
	float fallHeight = 30.0f;		//< 予告円の何メートル上から落とすか
	float fallSpeed = 40.0f;		//< 落下速度 (m/s)
	float colliderRadius = 2.0f;	//< 着弾時に出す判定の半径
	float impactHold = 0.2f;		//< 判定を出しておく秒数
};

/// <summary>リザルトシーンに直接置く予告円。位置と大きさは Transform で調整する</summary>
CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Meteorite Warning", Icon = "UI/Tool/event.png")
class MeteoriteWarning : public Actor {

public:

	MeteoriteWarning();
	~MeteoriteWarning() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void Start(const MeteoriteFallSettings& settings);

	bool IsFinished() const { return phase_ == Phase::Done; }

private:

	enum class Phase {
		Idle,		//< 号令待ち。円は出したまま
		Waiting,	//< delay 待ち
		Blinking,	//< 点滅中
		Falling,	//< 円を消して落下中
		Done,
	};

	void UpdateWaiting(float dt);
	void UpdateBlinking(float dt);
	void UpdateFalling(float dt);

	void DropMeteorite();

	void DisableGravity();

	MeteoriteFallSettings settings_{};

	Phase phase_ = Phase::Idle;
	float timer_ = 0.0f;
	int   blinkedCount_ = 0;

	std::shared_ptr<FallingMeteorite> meteorite_;

	struct MeteoriteWarningParam : CalyxEngine::SerializableObject {
		MeteoriteWarningParam() {
			AddField("delay", delay)
				.Category("Timing")
				.Tooltip("号令から点滅を始めるまでの秒数");
		}

		Guid ownerGuid_;
		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, ownerGuid_.ToString(), "Actor/Meteorite/MeteoriteWarning" };
		}

		float delay = 0.0f;
	};

	MeteoriteWarningParam param_;

public:

	void DerivativeGui() override;

};
