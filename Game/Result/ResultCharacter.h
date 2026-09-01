#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>

#include <string>

CALYX_OBJECT(Category = GameObject, DisplayName = "ResultCharacter", Icon = "Textures/player/player.png")
class ResultCharacter : public Actor {

public:

	ResultCharacter();
	~ResultCharacter() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void DisableGravity();

	void SetPosition(const CalyxEngine::Vector3& position) { GetWorldTransform().translation = position; }
	void UpdateScale(float dt);

private:

	// アニメーション時間
	float time_ = 0.0f;
	// スケーリング中かどうか
	bool isScaling_ = true;

};

