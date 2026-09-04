#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Player/Player.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Floater/BodyNode.h>

ResultManager::ResultManager()
    : Actor("debugCube.obj", "ResultManager") {}

void ResultManager::Initialize() {

    Actor::Initialize();

    DisableGravity();
    Actor::SetDrawEnable(false);

    InitializeActor();
}

void ResultManager::Update(float dt) {

    if (!initialized_) {
        return;
    }

    if (nextFloaterIndex_ >= ResultCarry::chain.size()) {
        return;
    }

    spawnTimer_ += dt;

    if (spawnTimer_ < spawnInterval_) {
        return;
    }

    spawnTimer_ = 0.0f;

    SpawnNextFloater();
}

void ResultManager::InitializeActor() {

    // ResultCarryにデータがない
    if (ResultCarry::chain.empty()) {
        return;
    }

    // プレイヤーを取得
    auto* ctx = SceneContext::Current();
    if (ctx) {
        player_ = ctx->FindFirst<Player>();
    }

    if (!player_) {
        return;
    }

    player_->Initialize();

    // ResultManagerの位置にPlayerを置く
    auto& playerWt = player_->GetWorldTransform();

    playerWt.translation =
        GetWorldTransform().translation;
    playerWt.translation.y = 0.5f;
    playerWt.rotationSource = RotationSource::Euler;
    playerWt.eulerRotation = { std::numbers::pi_v<float>, 0.0f, 0.0f };

    playerWt.Update();

    nextFloaterIndex_ = 1;
    spawnTimer_ = 0.0f;

    initialized_ = true;
}

void ResultManager::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void ResultManager::SpawnNextFloater() {

    if (!player_) {
        return;
    }

    if (nextFloaterIndex_ >= ResultCarry::chain.size()) {
        return;
    }

    const ChainMemberData& data =
        ResultCarry::chain[nextFloaterIndex_];

    const auto& playerWt =
        player_->GetWorldTransform();

    const CalyxEngine::Vector3 worldPos =
        playerWt.translation +
        BodyNode::RotateY(
            data.offset,
            playerWt.eulerRotation.y
        );

    std::shared_ptr<Floater> floater =
        SceneAPI::InstantiatePrefabRoot<Floater>(
            "Floater.prefab",
            worldPos
        );

    if (!floater) {
        return;
    }

    floater->Initialize();

    floater->RestoreChained();

    floater->SetChainedTransform(
        worldPos,
        playerWt.eulerRotation.y + data.localAngle
    );

    resultFloaters_.push_back(floater);

    nextFloaterIndex_++;
}