#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

#include <array>
#include <memory>
#include <nlohmann/json.hpp>

class Player;
class NumberUi;

CALYX_OBJECT(Category = GameObject, DisplayName = "PlayerTimeUis", Icon = "Textures/player/player.png")
class PlayerTimeUis : public Actor {

public:

	PlayerTimeUis();
	~PlayerTimeUis() override = default;

	void Initialize() override;
	void Update(float dt) override;

	// シリアライズ用インターフェース
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void DerivativeGui() override;

private:

	void InitializeActor();
	void DisableGravity();
	void AlarmTimer(float dt);

	// 加算された時間の表示
	void ShowAddNumber(int number);

	// 加算数字のフェード処理
	void UpdateAddNumber(float dt);

private:

	std::weak_ptr<Player> player_;

	// 通常の残り時間表示
	std::array<std::shared_ptr<NumberUi>, 2> numberUis_;

	// 時間が追加されたときだけ表示する数字
	std::shared_ptr<NumberUi> addNumberUi_;
	std::shared_ptr<NumberUi> addPlusUi_;

	int currentCount_ = 1;

	// カウントが始まったかどうか
	bool isCounting_ = false;

	// カウントの時間
	float countTime_ = 0.0f;
	// 拡縮時間
	float numberPulseTimer_ = 0.0f;

	bool Initialize_ = false;

	float countAlarmTime_ = 0.0f;

	// ============================
	// 加算数字のフェード
	// ============================
	bool isAddNumberVisible_ = false;
	float addNumberFadeTimer_ = 0.0f;

	// 調整可能なパラメータ
	struct TimeParam : CalyxEngine::SerializableObject {

		TimeParam() {

			AddField("addTimePerConnect", addTimePerConnect)
				.Category("Timing")
				.Tooltip("1人繋がったときに追加される時間 (秒)");

			AddField("maxTime", maxTime)
				.Category("Timing")
				.Tooltip("カウントの上限時間 (秒)");

			AddField("addNumberFadeTime", addNumberFadeTime)
				.Category("Timing")
				.Tooltip("追加時間の数字が消えるまでの時間 (秒)");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {
				CalyxEngine::ParamDomain::Game,
				"Player",
				"Actor/Player/PlayerTimeUis/TimeParam"
			};
		}

		float addTimePerConnect = 5.0f;
		float maxTime = 99.0f;

		// +5などの表示が消えるまで
		float addNumberFadeTime = 1.0f;

	} param_;
};