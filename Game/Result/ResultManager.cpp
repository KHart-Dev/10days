#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Player/Player.h>
#include <Game/Floater/FloaterManager.h>
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
		floaterManager_ = ctx->FindFirst<FloaterManager>();
    }

    if (player_) {
        player_->Initialize();
        player_->SetupResult();
    }
    if (floaterManager_) {
        floaterManager_->Initialize();
        floaterManager_->SetupResult();
    }

    auto& playerWt = player_->GetWorldTransform();
    playerWt.translation = GetWorldTransform().translation;
    playerWt.translation.y = 0.5f;
    playerWt.rotationSource = RotationSource::Euler;
    playerWt.Update();

    nextFloaterIndex_ = 1;
    spawnTimer_ = 0.0f;

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
}

void ResultManager::DisableGravity() {
    auto& movement = GetCharacterMovement();
    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);
}

void ResultManager::SpawnPlanets() {

    for (size_t i = 0; i < planets_.size(); ++i) {

        planets_[i] = SceneAPI::Instantiate<Planet>();
		planets_[i]->Initialize();
		auto& wt = planets_[i]->GetWorldTransform();
        float posX = static_cast<float>(i * 2);
        wt.translation = { (posX - 1.0f) * 29.0f, 0.0f, 0.0f };
		wt.rotationSource = RotationSource::Euler;
		wt.eulerRotation.x = std::numbers::pi_v<float> * 0.5f;
		wt.scale = { 15.0f, 15.0f, 15.0f };
    }
}
