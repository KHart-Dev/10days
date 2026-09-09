#include "PlayerTimeUis.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include "NumberUi.h"
#include "../Player.h"
#include <Game/Audio/GameAudio.h>
#include <algorithm>
#include <cmath>

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

	// 加算数字をフェード
	UpdateAddNumber(dt);

	if (auto player = player_.lock()) {

		// =================================
		// Floaterが追加された
		// =================================
		if (currentCount_ < player->GetConnectedCount()) {

			if (!isCounting_) {
				isCounting_ = true;
			}
			if (player->GetConnectedCount() == 2) {
				countTime_ += 5.0f;
			} else {
				countTime_ += param_.addTimePerConnect;
			}
			if (countTime_ > param_.maxTime) {
				countTime_ = param_.maxTime;
			}

			// =============================
			// 追加された数字を表示
			// =============================
			if (player->GetConnectedCount() == 2) {
				ShowAddNumber(
					static_cast<int>(5.0f)
				);
			} else {
				ShowAddNumber(
					static_cast<int>(param_.addTimePerConnect)
				);
			}
		}

		if (isCounting_) {

			countTime_ -= dt;
			if (countTime_ <= 3.0f) {
				AlarmTimer(dt);
				player->MemberShakeStart();
			} else {
				player->MemberShakeStop();
			}
			if (countTime_ <= 0.0f) {
				countTime_ = 0.0f;
				player->AllBreak();
			}
		}

		// カウントを更新
		currentCount_ = player->GetConnectedCount();

		// =================================
		// 通常の残り時間
		// =================================
		const int time = static_cast<int>(std::ceil(countTime_));
		const float posZ = player->GetWorldTransform().scale.x / 1.5f;

		// 0秒なら両方非表示
		if (time <= 0) {
			if (numberUis_[0]) {
				numberUis_[0]->SetDrawEnable(false);
			}
			if (numberUis_[1]) {
				numberUis_[1]->SetDrawEnable(false);
			}
		}
		// 1～9秒なら1の位だけ中央に表示
		else if (time < 10) {
			if (numberUis_[0]) {
				numberUis_[0]->SetDrawEnable(false);
			}

			if (numberUis_[1]) {
				numberUis_[1]->SetDrawEnable(true);
				numberUis_[1]->SetNumber(time);

				// 1桁なので中央
				numberUis_[1]->SetPosition(
					CalyxEngine::Vector3{ 0.0f, 0.1f, posZ }
				);
			}
		}
		// 10秒以上ならいつもの2桁表示
		else {
			if (numberUis_[0]) {
				numberUis_[0]->SetDrawEnable(true);
				numberUis_[0]->SetNumber((time / 10) % 10);

				numberUis_[0]->SetPosition(
					CalyxEngine::Vector3{ -0.5f, 0.1f, posZ }
				);
			}

			if (numberUis_[1]) {
				numberUis_[1]->SetDrawEnable(true);
				numberUis_[1]->SetNumber(time % 10);

				numberUis_[1]->SetPosition(
					CalyxEngine::Vector3{ 0.5f, 0.1f, posZ }
				);
			}
		}

		const CalyxEngine::Vector3 baseScale{ 0.5f, 0.9f, 1.0f };

		if (countTime_ > 0.0f && countTime_ <= 3.0f) {
			numberPulseTimer_ += dt;

			const float pulse = (std::sin(numberPulseTimer_ * 10.0f) + 1.0f) * 0.5f;
			const float scaleRate = 1.0f + pulse * 0.4f;

			for (auto& numberUi : numberUis_) {
				if (numberUi) {
					numberUi->SetScale({
						baseScale.x * scaleRate,
						baseScale.y * scaleRate,
						baseScale.z
						});
				}
			}
		} else {
			numberPulseTimer_ = 0.0f;

			for (auto& numberUi : numberUis_) {
				if (numberUi) {
					numberUi->SetScale(baseScale);
				}
			}
		}

		Actor::Update(dt);
	}
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
			numberUis_[i]->SetParent(player_.lock());
			numberUis_[i]->Initialize();
			numberUis_[i]->WtInitialize();
			float posX = static_cast<float>(i) - 0.5f;
			numberUis_[i]->SetPosition(CalyxEngine::Vector3(posX, 0.1f, player_.lock()->GetWorldTransform().scale.x / 1.5f));
		}
	}
	addNumberUi_ = SceneAPI::Instantiate<NumberUi>();
	if (addNumberUi_) {
		addNumberUi_->SetParent(player_.lock());
		addNumberUi_->Initialize();
		addNumberUi_->WtInitialize();
		addNumberUi_->SetPosition(CalyxEngine::Vector3{ 0.275f,0.1f,player_.lock()->GetWorldTransform().scale.x / 1.5f + 1.25f });
		addNumberUi_->SetScale(CalyxEngine::Vector3{ 0.35f,0.63f,0.5f });

		// 最初は透明
		addNumberUi_->SetColor({ 1.0f,1.0f,1.0f,0.0f });
		addNumberUi_->SetDrawEnable(false);
	}
	addPlusUi_ = SceneAPI::Instantiate<NumberUi>();
	if (addPlusUi_) {
		addPlusUi_->SetParent(player_.lock());
		addPlusUi_->Initialize();
		addPlusUi_->WtInitialize();
		// NumberUi::Initialize()でnumber.pngになるため、その後に+画像へ変更
		addPlusUi_->SetTexture("Textures/numbers/plus.png");
		addPlusUi_->SetPosition(CalyxEngine::Vector3{ -0.325f, 0.1f, player_.lock()->GetWorldTransform().scale.x / 1.5f + 1.25f });
		addPlusUi_->SetScale(CalyxEngine::Vector3{ 0.3f, 0.6f, 0.5f });
		addPlusUi_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		addPlusUi_->SetDrawEnable(false);
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

void PlayerTimeUis::AlarmTimer(float dt) {
	if (countAlarmTime_ > 0.0f) {
		countAlarmTime_ -= dt;
		return;
	}
	if (countTime_ <= 0.0f) return;
	countAlarmTime_ = 0.5f;
	GameAudio::PlaySe(GameAudio::kSeCount);
}

void PlayerTimeUis::ShowAddNumber(int number) {

	if (!addNumberUi_) {
		return;
	}
	// 数字を設定して表示
	addNumberUi_->SetNumber(number);
	addNumberFadeTimer_ = 0.0f;
	isAddNumberVisible_ = true;
	addNumberUi_->SetDrawEnable(true);
	addNumberUi_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	if (addPlusUi_) {
		addPlusUi_->SetDrawEnable(true);
		addPlusUi_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

void PlayerTimeUis::UpdateAddNumber(float dt) {

	if (!isAddNumberVisible_ || !addNumberUi_) {
		return;
	}

	// 時間経過に応じてフェードアウト
	addNumberFadeTimer_ += dt;
	const float fadeTime = std::fmax(param_.addNumberFadeTime, 0.001f);
	const float t = std::clamp(addNumberFadeTimer_ / fadeTime, 0.0f, 1.0f);
	const float alpha = 1.0f - t;
	addNumberUi_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	if (addPlusUi_) {
		addPlusUi_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	}
	if (t >= 1.0f) {
		isAddNumberVisible_ = false;

		addNumberUi_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		addNumberUi_->SetDrawEnable(false);

		if (addPlusUi_) {
			addPlusUi_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			addPlusUi_->SetDrawEnable(false);
		}
	}
}