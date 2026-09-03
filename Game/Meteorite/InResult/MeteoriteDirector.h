#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// game
#include <Game/Meteorite/InResult/MeteoriteWarning.h>

// std
#include <memory>
#include <vector>

/// <summary>リザルトシーンに1個置く。置かれている予告円を集めて号令を出すだけ</summary>
CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Meteorite Director", Icon = "UI/Tool/event.png")
class MeteoriteDirector : public Actor {

public:

	MeteoriteDirector();
	~MeteoriteDirector() override = default;

	void Initialize() override;
	void Update(float dt) override;

	/// 最終人数を出す合図用
	bool IsFinished() const;

private:

	/// シーンに置かれている MeteoriteWarning を集める
	void CollectWarnings();

	/// 集めた警告に号令を出す
	void StartAll();

	void DisableGravity();

	std::vector<std::shared_ptr<MeteoriteWarning>> warnings_;

	float timer_ = 0.0f;
	bool collected_ = false;
	bool started_ = false;

	struct MeteoriteDirectorParam : CalyxEngine::SerializableObject {
		MeteoriteDirectorParam() {
			AddField("startDelay", startDelay)
				.Category("Timing")
				.Tooltip("リザルトが始まってから号令を出すまでの秒数");

			AddField("blinkTimes", settings.blinkTimes)
				.Category("Warning")
				.Tooltip("何回点滅させてから落とすか");

			AddField("blinkPeriod", settings.blinkPeriod)
				.Category("Warning")
				.Tooltip("点滅1回ぶんの秒数");

			AddField("fallHeight", settings.fallHeight)
				.Category("Fall")
				.Tooltip("予告円の何メートル上から落とすか");

			AddField("fallSpeed", settings.fallSpeed)
				.Category("Fall")
				.Tooltip("落下速度 (m/s)");

			AddField("colliderRadius", settings.colliderRadius)
				.Category("Impact")
				.Tooltip("着弾時に出す判定の半径");

			AddField("impactHold", settings.impactHold)
				.Category("Impact")
				.Tooltip("判定を出しておく秒数。BreakChain が拾えるよう数フレームぶん要る");
		}

		Guid ownerGuid_;
		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, ownerGuid_.ToString(), "Actor/Meteorite/MeteoriteDirector" };
		}

		float startDelay = 1.0f;

		MeteoriteFallSettings settings{};
	};

	MeteoriteDirectorParam param_;

public:

	void DerivativeGui() override;

};
