#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>

#include <array>
#include <memory>
#include <vector>

#include <Tutorial/TutorialUi.h>

class Player;

CALYX_OBJECT(Category = GameObject,DisplayName = "Tutorial",Icon = "Textures/white1x1.png")
class Tutorial : public Actor {

public:

	Tutorial();
	~Tutorial() override = default;
	void Initialize() override;
	void Update(float dt) override;

private:

	std::weak_ptr<Player> player_;

	std::shared_ptr<TutorialUi> inputUi_;
	std::shared_ptr<TutorialUi> rotationUi_;

	float rotationTime_ = 0.0f;
	bool isLeftRotation_ = false;

};

