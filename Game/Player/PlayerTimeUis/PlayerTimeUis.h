#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <array>
#include <memory>

class Player;
class NumberUi;

CALYX_OBJECT(Category = GameObject, DisplayName = "PlayerTimeUis", Icon = "Textures/player/player.png")
class PlayerTimeUis : public Actor {

public:

	PlayerTimeUis();
	~PlayerTimeUis() override = default;

	void Initialize() override;
	void Update(float dt) override;


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

};

