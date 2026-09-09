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
#include <cstdint>

class MeteoriteForecast;
class UiSprite;

CALYX_OBJECT(Category = GameObject, DisplayName = "Player", Icon = "Textures/player/player.png")
class Player : public Actor {

public:

	Player();
	~Player() override;

	void Initialize() override;
	void Update(float dt) override;

	void MemberShakeStart();
	void MemberShakeStop();

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
	bool BeginRestoreChain();

	/// 天気予報を1回だけ生やす。ゲーム中の初回 Update から呼ぶ
	void SpawnForecast();

	/// 予報の表示状態に合わせてゲーム時間を止める / 戻す
	void UpdateForecastPause();

	void RestoreChainStep(float dt);

	/// 操作説明UIの2枚
	void InitializeControlUi();

	/// 最後に触った入力デバイスを見て、どちらを出すか決める
	void UpdateControlDevice();

	void ApplyControlUiLayout();

	/// 操作説明UIの配置と表示切り替え
	void ApplyControlUiSprite(const std::shared_ptr<UiSprite>& sprite, bool visible);

	/// 予報の呼び出し方を出す。操作説明とは別の場所に置くのでレイアウトも別
	void ApplyForecastUiSprite(bool visible);

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

			AddField("moveSpeedScaleGain", moveSpeedScaleGain)
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
			AddField("fieldYLimit", fieldZLimit)
				.Category("Movement")
				.Tooltip("フィールドのZの上限");
			AddField("fieldMinX", fieldMinX)
				.Category("Movement")
				.Tooltip("フィールドのXの下限。Floaterにも同じ値が渡る");

			AddField("shakeAmplitude", shakeAmplitude)
				.Category("Shake");
			AddField("shakeFrequency", shakeFrequency)
				.Category("Shake");
			AddField("shakeAngle", shakeAngle)
				.Category("Shake");

			AddField("controlUiCenterX", controlUiCenterX)
				.Category("ControlUI")
				.Tooltip("操作説明UIの中心X座標(px)。1280x720基準で左端が0");

			AddField("controlUiCenterY", controlUiCenterY)
				.Category("ControlUI")
				.Tooltip("操作説明UIの中心Y座標(px)。1280x720基準で上端が0");

			AddField("controlUiWidth", controlUiWidth)
				.Category("ControlUI")
				.Tooltip("操作説明UIの横サイズ(px)");

			AddField("controlUiHeight", controlUiHeight)
				.Category("ControlUI")
				.Tooltip("操作説明UIの縦サイズ(px)");

			AddField("controlUiOrderInLayer", controlUiOrderInLayer)
				.Category("ControlUI")
				.Tooltip("操作説明UIのOrderInLayer。大きいほど手前に出る");

			AddField("forecastUiCenterX", forecastUiCenterX)
				.Category("ForecastUI")
				.Tooltip("注意報の呼び出し方UIの中心X座標(px)。1280x720基準で左端が0");

			AddField("forecastUiCenterY", forecastUiCenterY)
				.Category("ForecastUI")
				.Tooltip("注意報の呼び出し方UIの中心Y座標(px)。1280x720基準で上端が0");

			AddField("forecastUiWidth", forecastUiWidth)
				.Category("ForecastUI")
				.Tooltip("注意報の呼び出し方UIの横サイズ(px)。絵は横長なので縦の3倍が目安");

			AddField("forecastUiHeight", forecastUiHeight)
				.Category("ForecastUI")
				.Tooltip("注意報の呼び出し方UIの縦サイズ(px)");

			AddField("forecastUiOrderInLayer", forecastUiOrderInLayer)
				.Category("ForecastUI")
				.Tooltip("注意報の呼び出し方UIのOrderInLayer。大きいほど手前に出る");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, "Player", "Actor/Player/PlayerParam" };
		}

		float moveSpeed = 5.0f;
		float rotSpeedDeg = 180.0f;
		float yawAcceleration = 10.0f;
		float heaviness = 0.10f;
		float moveSpeedScaleGain = 0.2f;

		float grabRadius = 1.2f;
		float armSeparation = 60.0f;
		float alignToChain = 0.0f;

		float reachRange = 3.0f;
		float reachSpeed = 0.5f;
		float reachSpread = 40.0f;

		float fieldMargin = 1.0f;
		float fieldZLimit = 18.0f;
		float fieldMinX = -20.0f;

		float shakeAmplitude = 0.06f;
		float shakeFrequency = 18.0f;
		float shakeAngle = 0.05f;

		// 操作説明UI。1280x720基準の画面座標で、アンカーは中心
		float controlUiCenterX = 1090.0f;
		float controlUiCenterY = 600.0f;
		float controlUiWidth = 320.0f;
		float controlUiHeight = 180.0f;
		int32_t controlUiOrderInLayer = 100;

		// 注意報の呼び出し方UI。操作説明のすぐ上に置く
		float forecastUiCenterX = 1090.0f;
		float forecastUiCenterY = 460.0f;
		float forecastUiWidth = 300.0f;
		float forecastUiHeight = 100.0f;
		int32_t forecastUiOrderInLayer = 100;
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

	bool  memberShaking_ = false;
	float shakeTimer_ = 0.0f;

	// 繋がらないときの切り分け用。距離で落ちているのか角度で落ちているのかを見る
	float debugNearestDist_ = -1.0f;
	int debugAngleRejects_ = 0;
	int debugBreakIndex_ = 1;

	bool isResultChain_ = false;
	bool resultMode_ = false;
	bool restored_ = false;

	// ResultSceneでFloaterを順番に出す
	size_t resultRestoreIndex_ = 1;

	float resultRestoreTimer_ = 0.0f;
	float resultRestoreInterval_ = 0.3f;

	CalyxEngine::EffectAsset HandConnectEffect_;
	CalyxEngine::EffectHandle HandConnectHandle_{};

	// ステージごとのPlayerとFloaterの基準サイズ（ステージごとにスケールを変えるので持つ。FloaterManagerにも渡してFloaterのサイズも変える）
	float stageScale_ = 1.0f;
	float stageClearLength_ = 5.0f;

	/// 操作説明UIをどちらのデバイス向けに出すか
	enum class ControlDevice {
		Keyboard,
		Gamepad,
	};

	std::shared_ptr<UiSprite> controlUIKeyboard_;
	std::shared_ptr<UiSprite> controlUIPad_;
	std::shared_ptr<UiSprite> forecastUI_;

	// 最初はキーボード。パッドを触った時点で入れ替わる
	ControlDevice controlDevice_ = ControlDevice::Keyboard;

public:
	// シリアライズ用インターフェース
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void DerivativeGui() override;
	void DisableGravity();

	// 接続されている人数を返す（UI や外部管理用）
	size_t GetConnectedCount() const { return chain_.size(); }
	void AllBreak();
	void ExportChain() const;
	void SetupResult();

	/// スタート時の天気予報を見せている間か。ゲーム進行を止めたいときに見る
	bool IsForecastWaiting() const;
	// リザルトで複製済みか
	bool IsResultChain() const { return isResultChain_; }

	// 接続中Floaterのいずれかの手が、指定した球の半径内にあるか
	bool IsConnectedFloaterHandInsideRadius(
		const CalyxEngine::Vector3& center,
		float radius) const;

	// 接続中Floaterの全ての手から、X座標の最小値と最大値を取得する。
	// 接続中Floaterが1人もいない場合はfalse。
	bool GetConnectedFloaterHandXRange(
		float& minX,
		float& maxX) const;

	// 接続中Floaterの手のX方向の最大距離
	float GetConnectedFloaterHandXDistance() const;
};
