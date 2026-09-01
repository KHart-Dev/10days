#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

ResultManager::ResultManager() {}

void ResultManager::Initialize() {
	BaseGameObject::Initialize();

	playerCharacter_ = SceneAPI::Instantiate<ResultCharacter>();
	playerCharacter_->Initialize();
}

void ResultManager::Update(float dt) {
	BaseGameObject::Update(dt);
	// 手を繋げた仲間の数を増やす処理
	timeToIncreaseFriends_ += dt;
	if (timeToIncreaseFriends_ >= 1.0f && numConnectedFriends_ > 0) { // 1秒ごとに増やす
		AddConnectedFriend();
		numConnectedFriends_--;
		timeToIncreaseFriends_ = 0.0f;
	}

	for (const auto& friendCharacter : connectedFriends_) {
		friendCharacter->UpdateScale(dt);
	}
}

void ResultManager::AddConnectedFriend() {
	// 手を繋げた仲間のキャラクターを追加する処理
	uint32_t count = counter_;
	counter_++;
	auto newFriend = SceneAPI::Instantiate<ResultCharacter>();
	newFriend->Initialize();
	bool isRightSide = (counter_ % 2 == 0);
	float offsetX = isRightSide ? 2.0f : -2.0f;
	float number = static_cast<float>(count / 2 + 1);
	newFriend->SetPosition(playerCharacter_->GetWorldPosition() + CalyxEngine::Vector3{ offsetX * number , 0.0f, 0.0f });
	connectedFriends_.push_back(newFriend);
}

void ResultManager::ApplyConfigFromJson(const nlohmann::json& j) {
	BaseGameObject::ApplyConfigFromJson(j);
}

void ResultManager::ExtractConfigToJson(nlohmann::json& j) const {
	BaseGameObject::ExtractConfigToJson(j);
}