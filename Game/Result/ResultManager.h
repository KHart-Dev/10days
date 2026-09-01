#pragma once

#include <Engine/Objects/3D/Actor/BaseGameObject.h>

#include "ResultCharacter.h"

#include <cstdint>
#include <memory>
#include <vector>

CALYX_OBJECT(Category = GameObject, DisplayName = "ResultManager", Icon = "UI/Tool/cube.dds")
class ResultManager : public BaseGameObject {

public:

	ResultManager();
	~ResultManager() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

private:
	
	// 手を繋げた仲間のキャラクターを追加する
	void AddConnectedFriend();

private:

	// プレイヤーのキャラクター
	std::shared_ptr<ResultCharacter> playerCharacter_;

	// 手を繋げた仲間のキャラクター
	std::vector<std::shared_ptr<ResultCharacter>> connectedFriends_;

	// 手を繋げた仲間の数
	uint32_t numConnectedFriends_ = 0;
	uint32_t counter_ = 0;

	// 手を繋げた仲間の数を増やす時間
	float timeToIncreaseFriends_ = 0.0f;

};

