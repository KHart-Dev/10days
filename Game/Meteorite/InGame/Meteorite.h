#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>

/// <summary>ゲーム中に xz 面を流れる隕石（流れ星）</summary>
CALYX_OBJECT(Category = GameObject, DisplayName = "Meteorite", Icon = "UI/Tool/cube.dds")
class Meteorite : public Actor {

public:

	Meteorite();
	~Meteorite() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void Launch(const CalyxEngine::Vector3& velocity, float colliderRadius);

	/// この円から出たら自分で Destroy する
	void SetBounds(const CalyxEngine::Vector3& center, float radius);

	bool IsDead() const { return dead_; }

private:

	void SetupCollider(float radius);
	void DisableGravity();
	bool IsOutOfBounds() const;

	CalyxEngine::Vector3 velocity_{};

	CalyxEngine::Vector3 boundsCenter_{};
	float boundsRadius_ = 60.0f;

	bool dead_ = false;

};
