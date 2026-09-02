#include "PlayerTimeUis.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include "NumberUi.h"
#include "../Player.h"

PlayerTimeUis::PlayerTimeUis()
	: Actor("plane.obj", "PlayerTimeUis") {}

void PlayerTimeUis::Initialize() {
	Actor::Initialize();
	DisableGravity();
	Actor::SetDrawEnable(false);
}

void PlayerTimeUis::Update(float dt) {
	InitializeActor();
	Actor::Update(dt);
}

void PlayerTimeUis::InitializeActor() {
	if (Initialize_) { return; }
	// プレイヤーを取得
	auto* ctx = SceneContext::Current();
	if (ctx) {
		player_ = ctx->FindFirst<Player>();
	}
	// 数字の初期化
	for (size_t i = 0; i < numberUis_.size(); i++) {
		numberUis_[i] = SceneAPI::Instantiate<NumberUi>();
		if (numberUis_[i]) {
			numberUis_[i]->SetParent(player_);
			numberUis_[i]->Initialize();
			float posX = static_cast<float>(i * 2) - 1.0f;
			numberUis_[i]->SetPosition(CalyxEngine::Vector3(posX, 0.5f, 0.0f));
		}
	}
	Initialize_ = true;
}

void PlayerTimeUis::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}