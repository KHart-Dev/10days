#include "Tutorial.h"

#include <Engine/Scene/utility/SceneUtility.h>

#include <Game/Player/Player.h>

Tutorial::Tutorial()
	: Actor("plane.obj", "Tutorial") {}

void Tutorial::Initialize() {
	Actor::Initialize();
	SetDrawEnable(false);

    auto& movement = GetCharacterMovement();
    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);

    auto* ctx = SceneContext::Current();
    if (ctx) {
        player_ = ctx->FindFirst<Player>();
    }

	inputUi_ = SceneAPI::Instantiate<TutorialUi>();
	if (inputUi_) {
		inputUi_->Initialize();
		inputUi_->SetParent(player_.lock());
		inputUi_->SetScale({ 1.0f, 0.5f, 1.0f });
		inputUi_->SetPosition({ 0.0f, -1.3f, -0.05f });
		inputUi_->SetTexture("Textures/player/LtRt.png");
	}

	rotationUi_ = SceneAPI::Instantiate<TutorialUi>();
	if (rotationUi_) {
		rotationUi_->Initialize();
		rotationUi_->SetParent(player_.lock());
		rotationUi_->SetPosition({ 0.0f, 0.0f, -0.05f });
		rotationUi_->SetTexture("Textures/player/rotate.png");
	}
}

void Tutorial::Update(float dt) {

	if (player_.lock()->GetFirstStageRotation()) {
		if (rotationUi_) {
			rotationUi_->SetDrawEnable(false);
		}
		if (inputUi_) {
			inputUi_->SetDrawEnable(false);
		}
	} else {
		if (!isLeftRotation_) {
			rotationTime_ += dt;
		} else {
			rotationTime_ -= dt;
		}

		if (rotationTime_ >= 1.0f) {
			isLeftRotation_ = true;
		} else if (rotationTime_ <= 0.0f) {
			isLeftRotation_ = false;
		}
		CalyxEngine::Quaternion rotation = CalyxEngine::Quaternion::MakeRotateY(rotationTime_ * std::numbers::pi_v<float>);
		if (rotationUi_) {
			rotationUi_->SetRotation(rotation);
		}
	}

	Actor::Update(dt);
}
