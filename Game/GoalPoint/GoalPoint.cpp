#include "GoalPoint.h"

#include "Game/Player/Player.h"
#include "Game/Scene/SceneFlow.h"

GoalPoint::GoalPoint()
	: Actor("debugCube.obj", "GoalPoint") {}

void GoalPoint::Initialize() {

	Actor::Initialize();
	DisableGravity();
}

void GoalPoint::Update(float dt) {

	Actor::Update(dt);
}

void GoalPoint::OnCollisionEnter(Collider* other) {
	BaseGameObject* owner = other ? other->GetOwner() : nullptr;
	if (auto* player = dynamic_cast<Player*>(owner)) {
		// ResultSceneへ渡す連結情報を保存
		player->ExportChain();
		// ResultSceneへ遷移
		SceneFlow::GoToResult();
	}
}

void GoalPoint::DisableGravity()
{
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}
