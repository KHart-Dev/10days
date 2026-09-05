#include "Thorn.h"

#include <Engine/Objects/Collider/Collider.h>

#include <Game/Player/Player.h>
#include <Game/Floater/Floater.h>

Thorn::Thorn() :Actor("plane.obj", "thorn") {
	//初期化時にスケールを小さくしておく
	worldTransform_.scale *= 0.5f;
	SetTexture("Textures/Obstacle/needle.png");
}
Thorn::~Thorn() = default;

void Thorn::Initialize() {
	Actor::Initialize();

	// 0～8 の9コマが横一列に並んだスプライトシート
	SetTexture("Textures/Obstacle/needle.png");

	currentFrame_ = 0;
	animationTimer_ = 0.0f;

	ApplyAnimationFrame();

	bloodEffect_.Load("BloodParticle");
}

void Thorn::Update(float dt)
{
	UpdateUvAnimation(dt);

	Actor::Update(dt);
}

// 衝突時処理
void Thorn::OnCollisionEnter(Collider* other){
	BaseGameObject* owner = other ? other->GetOwner() : nullptr;
	if (auto* floater = dynamic_cast<Floater*>(owner)) {
		if (floater->IsChained()) {
			SetTexture("Textures/Obstacle/needleBlood.png");
			bloodHandle_ = EffectAPI::Play(bloodEffect_, worldTransform_.GetWorldPosition() + CalyxEngine::Vector3(0.0f, 0.1f, 0.0f));
		}
	}
	if(auto* player = dynamic_cast<Player*>(owner)) {
		player->AllBreak();
	}
}

void Thorn::UpdateUvAnimation(float dt) {
	if (frameDuration_ <= 0.0f) {
		return;
	}

	animationTimer_ += dt;

	// dtが大きかった場合でもコマ飛びを正しく処理する
	while (animationTimer_ >= frameDuration_) {

		animationTimer_ -= frameDuration_;

		currentFrame_++;
		if (currentFrame_ >= kAnimationFrameCount) {
			currentFrame_ = 0;
		}

		ApplyAnimationFrame();
	}
}

void Thorn::ApplyAnimationFrame() {
	const float frameUvWidth =
		1.0f / static_cast<float>(kAnimationFrameCount);

	const CalyxEngine::Vector2 uvScale{
		frameUvWidth,
		1.0f
	};

	const CalyxEngine::Vector2 uvOffset{
		frameUvWidth * static_cast<float>(currentFrame_),
		0.0f
	};

	SetUvScale(uvScale);
	model_->uvTransform.translate = uvOffset;
}

