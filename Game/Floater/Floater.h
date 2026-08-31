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

private:

	void SetupCollider();
	void DisableGravity();

	void Drift(float dt);
	void BounceOnEdge();

	CalyxEngine::Vector3 driftDir_{};

	float spinRate_ = 0.0f;
	float driftSpeed_ = 1.2f;
	float spinSpeed_ = 1.2f;

};
