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

	std::shared_ptr<Player> player_;
	std::array<std::shared_ptr<NumberUi>, 2> numberUis_;


	int currentCount_ = 1;
	// カウントが始まったかどうか
	bool isCounting_ = false;
	// カウントの時間
	float countTime_ = 0.0f;
	bool Initialize_ = false;

	// 調整可能なパラメータ
	struct TimeParam : CalyxEngine::SerializableObject {
		TimeParam() {
			AddField("addTimePerConnect", addTimePerConnect)
				.Category("Timing")
				.Tooltip("1人繋がったときに追加される時間 (秒)");

			AddField("maxTime", maxTime)
				.Category("Timing")
				.Tooltip("カウントの上限時間 (秒)");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game, "Player", "Actor/Player/PlayerTimeUis/TimeParam" };
		}

		float addTimePerConnect = 5.0f;
		float maxTime = 99.0f;
	} param_;

};

