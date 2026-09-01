#include "ResultCharacter.h"

ResultCharacter::ResultCharacter()
	: Actor("ground.obj", "ResultCharacter") {
}

void ResultCharacter::Initialize() {
	Actor::Initialize();	
	DisableGravity();

	time_ = 0.0f;
	isScaling_ = true;
}

void ResultCharacter::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void ResultCharacter::Update(float dt) {
	Actor::Update(dt);
}

void ResultCharacter::UpdateScale(float dt) {

	if (!isScaling_) {
		return;
	} else {
		time_ += dt;
		if (time_ >= 1.0f) {
			isScaling_ = false;
			time_ = 1.0f;
		}
	}

	// スケールを徐々に大きくする処理
	auto& wt = GetWorldTransform();
	wt.scale = CalyxEngine::Vector3{ time_,  time_, time_ };
	wt.Update();
}