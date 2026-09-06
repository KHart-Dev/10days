#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Player/Player.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Result/Planet/Planet.h>
#include <Game/Floater/BodyNode.h>
#include <Game/UI/UiSprite.h>

#include <algorithm>
#include <cmath>
#include <numbers>

ResultManager::ResultManager()
    : Actor("debugCube.obj", "ResultManager") {}

void ResultManager::Initialize() {

    Actor::Initialize();

    DisableGravity();
    Actor::SetDrawEnable(false);

    InitializeActor();
    InitializeResultUi();
    InitializeDistanceUi();
}

void ResultManager::Update(float dt) {

    if (!initialized_) {
        return;
    }

    if (nextFloaterIndex_ < ResultCarry::chain.size()) {

        spawnTimer_ += dt;

        if (spawnTimer_ >= spawnInterval_) {
            spawnTimer_ = 0.0f;
            SpawnNextFloater();
        }
    }

    CheckStageClear();
    UpdateResultUi();
    UpdateDistanceUi();
}

void ResultManager::InitializeActor() {

    if (ResultCarry::chain.empty()) {
        return;
    }

    auto* ctx = SceneContext::Current();
    if (ctx) {
        player_ = ctx->FindFirst<Player>();
    }

    if (!player_) {
        return;
    }

    player_->Initialize();
    player_->SetupResult();

    auto& playerWt = player_->GetWorldTransform();
    playerWt.translation = GetWorldTransform().translation;
    playerWt.translation.y = 0.5f;
    playerWt.rotationSource = RotationSource::Euler;
    playerWt.Update();

    nextFloaterIndex_ = 1;
    spawnTimer_ = 0.0f;

    resultFloaters_.clear();
    planetTouched_ = { false, false };
    isClear_ = false;

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
    resultColorSprite_->SetPositionPx(640.0f, 250.0f);
    resultColorSprite_->SetSizePx(180.0f, 90.0f);
    resultColorSprite_->SetOrderInLayer(1000);
    resultColorSprite_->SetVisible(false);
}

void ResultManager::UpdateResultUi() {

    if (!resultColorSprite_) {
        return;
    }

    const bool spawnFinished =
        nextFloaterIndex_ >= ResultCarry::chain.size();

    resultColorSprite_->SetVisible(spawnFinished);

    if (!spawnFinished) {
        return;
    }

    if (isClear_) {
        resultColorSprite_->SetTexture("Textures/Result/clear.png");
    } else {
        resultColorSprite_->SetTexture("Textures/Result/gameover.png");
    }
}

void ResultManager::InitializeDistanceUi() {

    // 上段は目標距離用の赤系数字
    CreateDistanceSpriteGroup(
        targetDistanceSprites_,
        "Textures/Numbers/number.png"
    );

    // 下段は人間橋距離用の青系数字
    CreateDistanceSpriteGroup(
        bridgeDistanceSprites_,
        "Textures/Numbers/numberBlue.png"
    );

    LayoutDistanceSpriteGroup(
        targetDistanceSprites_,
        targetDistanceOnesX_,
        targetDistanceCenterY_
    );

    LayoutDistanceSpriteGroup(
        bridgeDistanceSprites_,
        bridgeDistanceOnesX_,
        bridgeDistanceCenterY_
    );

    SetDistanceSpriteValue(
        targetDistanceSprites_,
        static_cast<int>(
            std::roundf(
                std::fabs(ResultCarry::stageClearDirection)
            )
            )
    );

    SetDistanceSpriteValue(
        bridgeDistanceSprites_,
        0
    );
}

void ResultManager::CreateDistanceSpriteGroup(
    std::array<std::shared_ptr<UiSprite>, 2>& sprites,
    const char* texturePath) {

    for (std::shared_ptr<UiSprite>& sprite : sprites) {

        sprite =
            SceneAPI::Instantiate<UiSprite>(texturePath);

        if (!sprite) {
            continue;
        }

        sprite->SetAnchor({ 0.5f, 0.5f });
        sprite->SetSizePx(
            distanceDigitWidth_,
            distanceDigitHeight_
        );
        sprite->SetOrderInLayer(
            distanceUiOrderInLayer_
        );

        // number.png / numberBlue.png は
        // 0～9が横一列に並んだ10分割アトラス。
        // Distance表示自体は10の位・1の位の2Spriteだけ使う。
        sprite->SetHorizontalAtlasFrame(0, 10);
    }
}

void ResultManager::LayoutDistanceSpriteGroup(
    const std::array<std::shared_ptr<UiSprite>, 2>& sprites,
    float onesX,
    float centerY) {

    // [0] = 10の位
    // [1] =  1の位
    //
    // 1の位を必ず onesX に固定する。
    // 10の位はその左へ distanceDigitSpacing_ 分だけずらす。
    const std::array<float, 2> xPositions = {
        onesX - distanceDigitSpacing_,
        onesX
    };

    for (size_t i = 0; i < sprites.size(); ++i) {

        if (!sprites[i]) {
            continue;
        }

        sprites[i]->SetPositionPx(
            xPositions[i],
            centerY
        );

        sprites[i]->SetSizePx(
            distanceDigitWidth_,
            distanceDigitHeight_
        );
    }
}

void ResultManager::SetDistanceSpriteValue(
    const std::array<std::shared_ptr<UiSprite>, 2>& sprites,
    int value) {

    // Distance UIは2桁だけなので0～99へ制限する。
    value = std::clamp(value, 0, 99);

    const int tens = (value / 10) % 10;
    const int ones = value % 10;

    // 10の位
    if (sprites[0]) {
        const bool showTens = value >= 10;
        sprites[0]->SetVisible(showTens);

        if (showTens) {
            sprites[0]->SetHorizontalAtlasFrame(
                tens,
                10
            );
        }
    }

    // 1の位は常に表示し、X座標はLayout側で640に固定。
    if (sprites[1]) {
        sprites[1]->SetVisible(true);
        sprites[1]->SetHorizontalAtlasFrame(
            ones,
            10
        );
    }
}

int ResultManager::ComputeBridgeDistanceInt() const {

    if (!player_ || resultFloaters_.empty()) {
        return 0;
    }

    const auto& playerWt = player_->GetWorldTransform();

    // ResultSceneでの左右方向に沿った見かけの橋の長さを出す。
    // PlayerのY回転に追従するローカルX軸へ各手を射影し、max-min を距離とする。
    const CalyxEngine::Vector3 axis =
        BodyNode::RotateY(
            CalyxEngine::Vector3{ 1.0f, 0.0f, 0.0f },
            playerWt.eulerRotation.y
        );

    bool hasAnyHand = false;
    float minProj = 0.0f;
    float maxProj = 0.0f;

    for (const std::shared_ptr<Floater>& floater : resultFloaters_) {
        if (!floater) {
            continue;
        }

        for (int hand = 0; hand < BodyNode::kHandCount; ++hand) {
            const CalyxEngine::Vector3 p = floater->GetHandWorld(hand) - playerWt.translation;
            const float projection =
                p.x * axis.x +
                p.y * axis.y +
                p.z * axis.z;

            if (!hasAnyHand) {
                minProj = projection;
                maxProj = projection;
                hasAnyHand = true;
            } else {
                minProj = std::min(minProj, projection);
                maxProj = std::fmax(maxProj, projection);
            }
        }
    }

    if (!hasAnyHand) {
        return 0;
    }

    return static_cast<int>(std::roundf(maxProj - minProj));
}

void ResultManager::UpdateDistanceUi() {

    // Inspector等で位置調整値を変えた場合にも毎フレーム反映。
    LayoutDistanceSpriteGroup(
        targetDistanceSprites_,
        targetDistanceOnesX_,
        targetDistanceCenterY_
    );

    LayoutDistanceSpriteGroup(
        bridgeDistanceSprites_,
        bridgeDistanceOnesX_,
        bridgeDistanceCenterY_
    );

    // 上段: ステージの目標距離
    SetDistanceSpriteValue(
        targetDistanceSprites_,
        static_cast<int>(
            std::roundf(
                std::fabs(ResultCarry::stageClearDirection)
            )
            )
    );

    // 下段: 現在のFloaterの手の広がり
    SetDistanceSpriteValue(
        bridgeDistanceSprites_,
        ComputeBridgeDistanceInt()
    );
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

    const float clearDistance =
        std::fabs(ResultCarry::stageClearDirection);

    const float planetCenterDistance =
        planetRadius_ + clearDistance * 0.5f;

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

        planets_[i]->Initialize();

        auto& planetWt = planets_[i]->GetWorldTransform();
        planetWt.translation = positions[i];
        planetWt.rotationSource = RotationSource::Euler;
        planetWt.eulerRotation.x = std::numbers::pi_v<float> *0.5f;
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

    const bool clearNow =
        planetTouched_[0] &&
        planetTouched_[1];

    if (clearNow && !isClear_) {
        isClear_ = true;
        ResultCarry::stageClearCount++;
    }
}
