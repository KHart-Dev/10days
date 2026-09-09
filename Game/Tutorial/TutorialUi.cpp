#include "TutorialUi.h"

TutorialUi::TutorialUi()
	: Actor("plane.obj", "TutorialUi") {}

void TutorialUi::Initialize() {
	Actor::Initialize();

	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void TutorialUi::Update(float dt) {

	Actor::Update(dt);
}