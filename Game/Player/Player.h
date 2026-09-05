#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Scene/Reference/SceneObjectReference.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// game
#include <Demo/Input/PlayerInput.h>
#include <Game/Floater/BodyNode.h>
#include <Game/Floater/FloaterManager.h>
#include <Game/Floater/ChainMemberData.h>

// std
#include <vector>
#include <memory>

class MeteoriteForecast;

CALYX_OBJECT(Category = GameObject, DisplayName = "Player", Icon = "Textures/player/player.png")
class Player : public Actor {

public:

	Player();
	~Player() override;

	void Initialize() override;
	void Update(float dt) override;

private:

	/// <summary>塊に繋がった1人</summary>
	struct Member : ChainMemberData {
		std::shared_ptr<Floater> floater;                 // 自機自身は nullptr
	};

	struct HandAnchor {
		CalyxEngine::Vector3 pos{};
		CalyxEngine::Vector3 prevPos{};
		// 体の中心から手へ向かうワールドのベクトル
		CalyxEngine::Vector3 arm{};
		int member = 0;
		int hand = 0;
	};

	void ClampToField();

	// 塊の手を全部ワールドへ出す
	void BuildHandAnchors();
	void CheckConnect(FloaterManager& manager, float dt);
	void ReachToNearestHand(Floater& floater, float reachSq, float dt);
	void Attach(const std::shared_ptr<Floater>& floater, const HandAnchor& anchor, int ownHand);
	/// <summary>繋がった全員のワールド姿勢を、自機の姿勢から組み直す</summary>
	void ApplyChainTransforms();
	float CurrentTurnSpeed() const;
	bool BreakChain(FloaterManager& manager);

	bool RestoreChain();

	/// 天気予報を1回だけ生やす。ゲーム中の初回 Update から呼ぶ
	void SpawnForecast();

	/// 予報の表示状態に合わせてゲーム時間を止める / 戻す
	void UpdateForecastPause();

	void RestoreChainStep(float dt);

	PlayerInput input_;

	// 回転慣性（Y軸）
	float yawVelocity_ = 0.0f;         // 現在の角速度 (rad/s)

	// 保存可能なパラメータは SerializableObject を使ってまとめる
	struct PlayerParam : CalyxEngine::SerializableObject {
		PlayerParam() {
			AddField("moveSpeed", moveSpeed)
				.Category("Movement")
				.Tooltip("移動速度 (m/s)");

			AddField("rotSpeedDeg", rotSpeedDeg)
				.Category("Movement")
				.Tooltip("回転速度 (deg/s)");

			AddField("yawAcceleration", yawAcceleration)
				.Category("Movement")
				.Tooltip("回転慣性の追従係数");

			AddField("heaviness", heaviness)
				.Category("Movement")
				.Tooltip("繋いだ人数ぶんの重さ。回転が遅くなる");

			AddField("grabRadius", grabRadius)
				.Category("Connect")
				.Tooltip("手を繋ぐ半径");

			AddField("armSeparation", armSeparation)
				.Category("Connect")
				.Tooltip("2本の腕をこれ以上開いていないと繋がない");

			AddField("alignToChain", alignToChain)
				.Category("Connect")
				.Tooltip("連結時に親の向きへ寄せる割合。どのくらい補正するのか");

			AddField("reachRange", reachRange)
				.Category("Reach")
				.Tooltip("手を差し出し始める距離");

			AddField("reachSpeed", reachSpeed)
				.Category("Reach")
				.Tooltip("手を差し出すときの回転速度");

			AddField("reachSpread", reachSpread)
				.Category("Reach")
				.Tooltip("差し出す向きの個人差");

			AddField("fieldMargin", fieldMargin)
				.Category("Movement")
				.Tooltip("フィールドの縁からどれだけ内側で止まるか");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, "Player", "Actor/Player/PlayerParam" };
		}

		float moveSpeed = 5.0f;
		float rotSpeedDeg = 180.0f;
		float yawAcceleration = 10.0f;
		float heaviness = 0.10f;

		float grabRadius = 1.2f;
		float armSeparation = 60.0f;
		float alignToChain = 0.0f;

		float reachRange = 3.0f;
		float reachSpeed = 0.5f;
		float reachSpread = 40.0f;

		float fieldMargin = 1.0f;
	};

	PlayerParam param_;
	CalyxEngine::SceneObjectRef<FloaterManager> floaterManager_;

	// 天気予報。自分の子として持ち、毎フレーム カメラの前へ投影される
	std::shared_ptr<MeteoriteForecast> forecast_;

	// 予報のために TimeScale を 0 にしているか
	bool forecastPaused_ = false;

	std::vector<Member> chain_;
	std::vector<HandAnchor> handAnchors_;
	std::vector<float> usedAngles_;
	std::vector<bool> 	breakMarks_;
	std::vector<int> 	remap_;

	CalyxEngine::Vector3 prevSelfPos_{};
	float prevSelfYaw_ = 0.0f;

	// 繋がらないときの切り分け用。距離で落ちているのか角度で落ちているのかを見る
	float debugNearestDist_ = -1.0f;
	int debugAngleRejects_ = 0;
	int debugBreakIndex_ = 1;

	bool resultMode_ = false;
	bool restored_ = false;

	// ResultSceneでFloaterを順番に出す
	size_t resultRestoreIndex_ = 1;

	float resultRestoreTimer_ = 0.0f;
	float resultRestoreInterval_ = 0.2f;

	CalyxEngine::EffectAsset HandConnectEffect_;
	CalyxEngine::EffectHandle HandConnectHandle_{};

public:
	// シリアライズ用インターフェース
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void DerivativeGui() override;

	// 接続されている人数を返す（UI や外部管理用）
	size_t GetConnectedCount() const { return chain_.size(); }
	void AllBreak();
	void ExportChain() const;

	void SetupResult();

	/// スタート時の天気予報を見せている間か。ゲーム進行を止めたいときに見る
	bool IsForecastWaiting() const;

};
