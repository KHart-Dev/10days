#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <memory>

CALYX_OBJECT(Category = GameObject, DisplayName = "GoalPoint", Icon = "UI/Tool/cube.dds")
class GoalPoint : public Actor {

public:

	GoalPoint();
	~GoalPoint() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void OnCollisionEnter([[maybe_unused]] Collider* other)override;
	std::string_view GetObjectClassName() const override { return "GoalPoint"; }

private:

	void DisableGravity();

};

