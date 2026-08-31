#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>

// game
#include <Demo/Input/PlayerInput.h>

CALYX_OBJECT(Category = GameObject, DisplayName = "Player", Icon = "UI/Tool/cube.dds")
class Player : public Actor {

public:

	Player();
	~Player() override = default;

	void Initialize() override;
	void Update(float dt) override;

private:

	PlayerInput input_;



};

