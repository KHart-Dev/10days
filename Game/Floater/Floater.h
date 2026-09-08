#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Scene/Utility/SceneUtility.h>

CALYX_OBJECT(Category = GameObject, DisplayName = "Floater", Icon = "UI/Tool/cube.dds")
class Floater : public Actor {

public:

	Floater();
	~Floater() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void OnCollisionEnter([[maybe_unused]] Collider* other)override;

	void SetDriftSpeed(float speed) { driftSpeed_ = speed; }
	void SetSpinSpeed(float speed) { spinSpeed_ = speed; }
	//　漂う範囲
	void SetBounds(const CalyxEngine::Vector3& center, float radius) {
		boundsCenter_ = center;
		boundsRadius_ = radius;
	}

	/// <summary>Player と同じ壁の値を受け取る</summary>
	void SetFieldLimits(float minX, float zLimit) {
		fieldMinX_ = minX;
		fieldZLimit_ = zLimit;
	}

	/// <summary>湧かせた座標を入れ終えてから呼ぶ。範囲外なら中心へ向かう状態で始まる</summary>
	void BeginDrift();

	float GetYaw() const;
	CalyxEngine::Vector3 GetArmWorld(int hand) const; // 中心から手へのベクトル
	CalyxEngine::Vector3 GetHandWorld(int hand) const; // 手のワールド座標
	float GetReachBias() const { return reachBias_; }
	bool CanConnect() const;

	void SetStageScale(float scale) { stageScale_ = scale; }
	void ApplyStageScale();

	bool IsChained() const { return chained_; }
	void MarkChained(int hand);
	void RestoreChained();

	bool IsBreakMarked() const { return breakMark_; }
	void MarkBreak();
	void Unchain();

	/// <summary>連結中の姿勢を外から与える</summary>
	/// <remarks>親子付けは使わない。ピッチを毎フレーム固定値で書き直すので、
	/// 親の回転がピッチ済みの軸に乗って他軸が回ることが無い</remarks>
	void SetChainedTransform(const CalyxEngine::Vector3& pos, float worldYaw);

	void ReachTowardArmAngle(int hand, float targetArmAngle, float step);

private:

	void SetupCollider();
	void DisableGravity();

	void Drift(float dt);
	void BounceOnEdge();
	bool IsInsideField() const;

	void ApplyChainedLook();

	CalyxEngine::Vector3 driftDir_{};

	float spinRate_ = 0.0f;
	float driftSpeed_ = 1.2f;
	float spinSpeed_ = 1.2f;
	float reachBias_ = 0.0f;

	bool chained_ = false;
	bool breakMark_ = false;
	float breakedCooltime_ = 0.0f; // 壊れた後、すぐに繋がらないように
	int connectedHand_ = -1;

	// 範囲外に湧いた個体が、範囲へ入るまでの間だけ立つ。
	// 立っている間は壁で跳ね返さず、代わりに中心へ向かって加速する
	bool entering_ = false;

	CalyxEngine::Vector3 boundsCenter_{};
	float boundsRadius_ = 30.0f;

	// Player の ClampToField と同じ壁。Player から FloaterManager 経由で入る
	float fieldMinX_ = -20.0f;
	float fieldZLimit_ = 18.0f;

	float stageScale_ = 1.0f;

	CalyxEngine::EffectAsset explosionEffect_;
	CalyxEngine::EffectHandle explosionHandle_{};

};
