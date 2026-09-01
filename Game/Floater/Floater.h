#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>


CALYX_OBJECT(Category = GameObject, DisplayName = "Floater", Icon = "UI/Tool/cube.dds")
class Floater : public Actor {

public:

	Floater();
	~Floater() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void SetDriftSpeed(float speed) { driftSpeed_ = speed; }
	void SetSpinSpeed(float speed) { spinSpeed_ = speed; }
	//　漂う範囲
	void SetBounds(const CalyxEngine::Vector3& center, const CalyxEngine::Vector3& half) {
		boundsCenter_ = center;
		boundsHalf_ = half;
	}

	float GetYaw() const;
	CalyxEngine::Vector3 GetArmWorld(int hand) const; // 中心から手へのベクトル
	CalyxEngine::Vector3 GetHandWorld(int hand) const; // 手のワールド座標
	float GetReachBias() const { return reachBias_; }

	bool IsChained() const { return chained_; }
	void MarkChained();

	void ReachTowardArmAngle(int hand, float targetArmAngle, float step);

private:

	void SetupCollider();
	void DisableGravity();

	void Drift(float dt);
	void BounceOnEdge();

	CalyxEngine::Vector3 driftDir_{};

	float spinRate_ = 0.0f;
	float driftSpeed_ = 1.2f;
	float spinSpeed_ = 1.2f;
	float reachBias_ = 0.0f;

	bool chained_ = false;

	CalyxEngine::Vector3 boundsCenter_{};
	CalyxEngine::Vector3 boundsHalf_{ 30.0f, 0.0f, 30.0f };

};
