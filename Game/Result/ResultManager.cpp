#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Player/Player.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Result/Planet/Planet.h>
#include <Game/Floater/BodyNode.h>
#include <Game/UI/UiSprite.h>

#include <cmath>

ResultManager::ResultManager()
    : Actor("debugCube.obj", "ResultManager") {}

void ResultManager::Initialize() {

    Actor::Initialize();

    DisableGravity();
    Actor::SetDrawEnable(false);

    InitializeActor();
    InitializeResultUi();
}

void ResultManager::Update(float dt) {

    if (!initialized_) {
        return;
    }

    // Floaterは今まで通り順番に生成する。
    // 全員生成済みでも、下のクリア判定は毎フレーム続ける。
    if (nextFloaterIndex_ < ResultCarry::chain.size()) {

        spawnTimer_ += dt;

        if (spawnTimer_ >= spawnInterval_) {
            spawnTimer_ = 0.0f;
            SpawnNextFloater();
        }
    }

    CheckStageClear();
    UpdateResultUi();
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
    player_->SetupResult();

    // ResultManagerの位置にPlayerを置く
    auto& playerWt = player_->GetWorldTransform();

    playerWt.translation =
        GetWorldTransform().translation;
    playerWt.translation.y = 0.5f;
    playerWt.rotationSource = RotationSource::Euler;

    playerWt.Update();

    nextFloaterIndex_ = 1;
    spawnTimer_ = 0.0f;

    resultFloaters_.clear();
    planetTouched_ = { false, false };
    isClear_ = false;

    // ResultCarry::stageClearDirectionを使って、
    // Playerの左右に2つのPlanetを生成する。
    SpawnPlanets();

    initialized_ = true;
}


void ResultManager::InitializeResultUi() {

    resultColorSprite_ =
        SceneAPI::Instantiate<UiSprite>("Textures/white1x1.dds");

    if (!resultColorSprite_) {
        return;
    }

    resultColorSprite_->SetAnchor({ 0.5f, 0.5f });

    // ひとまず画面上部中央に結果確認用の四角として表示。
    // 必要なら位置・サイズはここを調整してください。
    resultColorSprite_->SetPositionPx(640.0f, 90.0f);
    resultColorSprite_->SetSizePx(120.0f, 60.0f);

    // UIの手前側へ
    resultColorSprite_->SetOrderInLayer(1000);

    // Floaterが全員出るまではまだ結果を出さない。
    resultColorSprite_->SetVisible(false);
}

void ResultManager::UpdateResultUi() {

    if (!resultColorSprite_) {
        return;
    }

    // 全Floaterの生成が終わるまでは判定結果を見せない。
    const bool spawnFinished =
        nextFloaterIndex_ >= ResultCarry::chain.size();

    resultColorSprite_->SetVisible(spawnFinished);

    if (!spawnFinished) {
        return;
    }

    if (isClear_) {
        // CLEAR = 緑
        resultColorSprite_->SetColorRGBA(
            0.0f,
            1.0f,
            0.0f,
            1.0f
        );
    } else {
        // FAILED = 赤
        resultColorSprite_->SetColorRGBA(
            1.0f,
            0.0f,
            0.0f,
            1.0f
        );
    }
}

void ResultManager::DisableGravity() {
    auto& movement = GetCharacterMovement();
    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);
}

void ResultManager::SpawnPlanets() {

    if (!player_) {
        return;
    }

    const auto& playerWt = player_->GetWorldTransform();

    // stageClearDirection は「左右Planetの内側どうしのクリア距離」として扱う。
    // Player中心からPlanet中心までの距離は
    //
    //     Planet半径 + クリア距離の半分
    //
    // とする。
    const float clearDistance =
        std::fabs(ResultCarry::stageClearDirection);

    const float planetCenterDistance =
        planetRadius_ + clearDistance * 0.5f;

    // ResultManager/PlayerのY回転に追従して左右軸を作る。
    const CalyxEngine::Vector3 leftOffset =
        BodyNode::RotateY(
            CalyxEngine::Vector3{ -planetCenterDistance, 0.0f, 0.0f },
            playerWt.eulerRotation.y
        );

    const CalyxEngine::Vector3 rightOffset =
        BodyNode::RotateY(
            CalyxEngine::Vector3{ planetCenterDistance, 0.0f, 0.0f },
            playerWt.eulerRotation.y
        );

    const std::array<CalyxEngine::Vector3, 2> positions = {
        playerWt.translation + leftOffset,
        playerWt.translation + rightOffset
    };

    for (size_t i = 0; i < planets_.size(); ++i) {

        planets_[i] = SceneAPI::Instantiate<Planet>();

        if (!planets_[i]) {
            continue;
        }

        // SceneAPI::InstantiateはInitializeを自動では呼ばないため明示的に呼ぶ。
        planets_[i]->Initialize();

        auto& planetWt = planets_[i]->GetWorldTransform();
        planetWt.translation = positions[i];
        planetWt.rotationSource = RotationSource::Euler;
        planetWt.eulerRotation.x = std::numbers::pi_v<float> * 0.5f;
        planetWt.Update();

        planets_[i]->SetRadius(planetRadius_);
    }
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

void ResultManager::CheckStageClear() {

    planetTouched_ = { false, false };

    // resultFloaters_の両手を調べる。
    for (const std::shared_ptr<Floater>& floater : resultFloaters_) {

        if (!floater) {
            continue;
        }

        for (int hand = 0; hand < BodyNode::kHandCount; ++hand) {

            const CalyxEngine::Vector3 handPosition =
                floater->GetHandWorld(hand);

            for (size_t planetIndex = 0;
                planetIndex < planets_.size();
                ++planetIndex) {

                if (!planets_[planetIndex]) {
                    continue;
                }

                if (planets_[planetIndex]->ContainsPoint(handPosition)) {
                    planetTouched_[planetIndex] = true;
                }
            }
        }
    }

    // 左右両方のPlanetにFloaterの手が入っていればステージクリア。
    const bool clearNow =
        planetTouched_[0] &&
        planetTouched_[1];

    // 一度クリアしたら確定。手がPlanetから離れてもクリア状態は保持する。
    if (clearNow && !isClear_) {
        isClear_ = true;
        ResultCarry::stageClearCount++;
    }
}