#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>

/// <summary>リザルトで真上から落ちてくる隕石。1地点につき1個</summary>
CALYX_OBJECT(Category = GameObject, DisplayName = "Falling Meteorite", Icon = "UI/Tool/cube.dds")
class FallingMeteorite : public Actor {

public:

	FallingMeteorite();
	~FallingMeteorite() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void Fall(const CalyxEngine::Vector3& impactPos,
			  float fallHeight, float speed,
			  float colliderRadius, float impactHold);

	/// 着弾して判定を出し終えたか
	bool IsFinished() const { return phase_ == Phase::Done; }

private:

	enum class Phase {
		Idle,
		Falling,
		Impact,		//< 判定を出している最中
		Done,
	};

	void SetupCollider(float radius);
	void DisableGravity();

	// 当たり判定の切り替え
	void EnableCollider(bool enable);

	CalyxEngine::Vector3 impactPos_{};

	Phase phase_ = Phase::Idle;
	float fallSpeed_ = 0.0f;
	float impactHold_ = 0.0f;
	float timer_ = 0.0f;

};
