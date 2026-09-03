#include "PlayerTimeUis.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include "NumberUi.h"
#include "../Player.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

PlayerTimeUis::PlayerTimeUis()
	: Actor("plane.obj", "PlayerTimeUis") {}

void PlayerTimeUis::Initialize() {
	Actor::Initialize();
	InitializeActor();
	DisableGravity();
	Actor::SetDrawEnable(false);
}
void PlayerTimeUis::Update(float dt) {

	// カウントが上がっていたら（接続時に追加する時間は param_.addTimePerConnect、上限は param_.maxTime）
	if (currentCount_ < player_->GetConnectedCount()) {
		if (!isCounting_) {
			isCounting_ = true;
		}
		countTime_ += param_.addTimePerConnect;
		if (countTime_ > param_.maxTime) {
			countTime_ = param_.maxTime;
		}
	}

	if (isCounting_) {
		countTime_ -= dt;

		if (countTime_ <= 0.0f) {
			countTime_ = 0.0f;
			player_->AllBreak();
		}
	}

	// カウントを更新
	currentCount_ = player_->GetConnectedCount();

	// 表示用に整数へ変換
	int time = static_cast<int>(countTime_);

	// 数字の表示を更新
	for (size_t i = 0; i < numberUis_.size(); i++) {

		int number = 0;

		if (i == 0) {
			// 2桁目
			number = (time / 10) % 10;
		} else if (i == 1) {
			// 1桁目
			number = time % 10;
		}

		/*if (numberUis_[i]) {
			numberUis_[i]->SetNumber(number);
		}*/
	}

	Actor::Update(dt);
}

void PlayerTimeUis::ApplyConfigFromJson(const nlohmann::json& j) {
	Actor::ApplyConfigFromJson(j);

	const std::string typeKey(GetTypeName());
	const nlohmann::json* src = &j;
	if (j.contains(typeKey)) {
		src = &j.at(typeKey);
	}

	param_.addTimePerConnect = src->value("addTimePerConnect", param_.addTimePerConnect);
	param_.maxTime = src->value("maxTime", param_.maxTime);
}

void PlayerTimeUis::ExtractConfigToJson(nlohmann::json& j) const {
	Actor::ExtractConfigToJson(j);

	const std::string typeKey(GetTypeName());
	nlohmann::json derived;
	derived["addTimePerConnect"] = param_.addTimePerConnect;
	derived["maxTime"] = param_.maxTime;
	if (!derived.empty()) {
		j[typeKey] = std::move(derived);
	}
}

void PlayerTimeUis::DerivativeGui() {
	param_.ShowGui();
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
			float posX = static_cast<float>(i) - 0.5f;
			numberUis_[i]->SetPosition(CalyxEngine::Vector3(posX, 0.5f, 3.0f));
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