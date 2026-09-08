#include "ResultManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Player/Player.h>
#include <Game/Floater/FloaterManager.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Result/Planet/Planet.h>
#include <Game/Meteorite/InResult/MeteoriteDirector.h>
#include <Game/Floater/BodyNode.h>
#include <Game/UI/UiSprite.h>
#include <Game/Audio/GameAudio.h>

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
    UpdatePlanetTouchState();
    UpdateResultUi(dt);
    UpdateDistanceUi();
}

void ResultManager::InitializeActor() {

    auto* ctx = SceneContext::Current();
    if (ctx) {
        player_ = ctx->FindFirst<Player>();
        floaterManager_ = ctx->FindFirst<FloaterManager>();
        director_ = ctx->FindFirst<MeteoriteDirector>();
    }

    if (auto player = player_.lock()) {

        player->SetupResult();
        auto& playerWt = player->GetWorldTransform();
        playerWt.translation = GetWorldTransform().translation;
        playerWt.translation.y = 0.5f;
        playerWt.rotationSource = RotationSource::Euler;
        playerWt.Update();
    }

    // FloaterManager の resultMode_ はシリアライズされないのでシーンからは入らない。
    // 呼ばないとリザルトでゲームBGMが鳴り、Floater が撒かれる
    if (auto floaterManager = floaterManager_.lock()) {
        floaterManager->SetupResult();
    }

    planetTouched_ = { false, false };
    resultFixed_ = false;
    isClear_ = false;

    SpawnPlanets();

    initialized_ = true;
}

void ResultManager::InitializeResultUi() {

    resultClearSprite_ =
        SceneAPI::Instantiate<UiSprite>("Textures/white1x1.dds");

    if (!resultClearSprite_) {
        return;
    }

    resultClearSprite_->SetAnchor({ 0.5f, 0.5f });
    resultClearSprite_->SetPositionPx(640.0f, 250.0f);
    resultClearSprite_->SetSizePx(300.0f, 150.0f);
    resultClearSprite_->SetOrderInLayer(1000);
    resultClearSprite_->SetVisible(false);
}

void ResultManager::UpdateResultUi(float dt) {

    if (!resultClearSprite_) {
        return;
    }

    // 結果が確定してからCLEAR / GAMEOVERを表示する。
    resultClearSprite_->SetVisible(resultFixed_);

    if (!resultFixed_) {
        return;
    } else {
        clearAlpha_ += dt;
		clearAlpha_ = std::clamp(clearAlpha_, 0.0f, 1.0f);
		resultClearSprite_->SetColorRGBA(1.0f, 1.0f, 1.0f, clearAlpha_);
    }

    if (isClear_) {
        resultClearSprite_->SetTexture("Textures/Result/clear.png");
        if (!isResultOnce_) return;
        GameAudio::PlaySe(GameAudio::kSeClear);
        isResultOnce_ = false;
    } else {
        resultClearSprite_->SetTexture("Textures/Result/gameover.png");
        if (!isResultOnce_) return;
        GameAudio::PlaySe(GameAudio::kSeGameover,0.3f);
        isResultOnce_ = false;
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

    // =========================================
    // UI配置
    // =========================================

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


    // =========================================
    // 上段 : ステージの目標距離
    // =========================================

    const float targetDistance =
        std::fabs(
            ResultCarry::stageClearDirection
        );

    SetDistanceSpriteValue(
        targetDistanceSprites_,
        static_cast<int>(
            std::roundf(targetDistance)
            )
    );


    // =========================================
    // 下段 : 実際の人間橋距離
    // =========================================

    const std::shared_ptr<Player> player =
        player_.lock();

    if (!player) {

        bridgeMinHandX_ = 0.0f;
        bridgeMaxHandX_ = 0.0f;
        bridgeWorldDistance_ = 0.0f;
        bridgeConvertedDistance_ = 0.0f;

        SetDistanceSpriteValue(
            bridgeDistanceSprites_,
            0
        );

        return;
    }


    // =========================================
    // 接続中Floaterの手の
    // 最小X / 最大Xを取得
    // =========================================

    if (!player->GetConnectedFloaterHandXRange(
        bridgeMinHandX_,
        bridgeMaxHandX_)) {

        bridgeWorldDistance_ = 0.0f;
        bridgeConvertedDistance_ = 0.0f;

        SetDistanceSpriteValue(
            bridgeDistanceSprites_,
            0
        );

        return;
    }


    // =========================================
    // 手から手までの実際のX距離
    // =========================================

    bridgeWorldDistance_ =
        bridgeMaxHandX_ -
        bridgeMinHandX_;


    // =========================================
    // 左右Planetの有効距離を計算
    //
    // Planet中心間距離
    //   - 左Planet半径
    //   - 右Planet半径
    //
    // つまりPlanetの内側～内側の距離
    // =========================================

    if (!planets_[0] || !planets_[1]) {

        planetEffectiveDistance_ = 0.0f;
        bridgeConvertedDistance_ = 0.0f;

        SetDistanceSpriteValue(
            bridgeDistanceSprites_,
            0
        );

        return;
    }

    const CalyxEngine::Vector3 leftPlanetPos =
        planets_[0]->
        GetWorldTransform().
        GetWorldPosition();

    const CalyxEngine::Vector3 rightPlanetPos =
        planets_[1]->
        GetWorldTransform().
        GetWorldPosition();


    const float planetCenterDistance =
        std::fabs(
            rightPlanetPos.x -
            leftPlanetPos.x
        );


    planetEffectiveDistance_ =
        planetCenterDistance -
        planetRadius_ * 2.0f;


    // 0除算防止
    if (planetEffectiveDistance_ <= 0.0f) {

        bridgeConvertedDistance_ = 0.0f;

        SetDistanceSpriteValue(
            bridgeDistanceSprites_,
            0
        );

        return;
    }


    // =========================================
    // Result上の距離を
    // stageClearDirection基準へ変換
    //
    // bridgeWorldDistance
    // --------------------- × stageClearDirection
    // planetEffectiveDistance
    // =========================================

    const float distanceRatio =
        bridgeWorldDistance_ /
        planetEffectiveDistance_;


    bridgeConvertedDistance_ =
        distanceRatio *
        targetDistance;


    // =========================================
    // 青い数字へ表示
    // =========================================

    SetDistanceSpriteValue(
        bridgeDistanceSprites_,
        static_cast<int>(
            std::roundf(
                bridgeConvertedDistance_
            )
            )
    );
}

void ResultManager::UpdatePlanetTouchState() {

    std::shared_ptr<Player> player = player_.lock();

    if (!player) {
        planetTouched_ = { false, false };
        resultFixed_ = false;
        isClear_ = false;
        return;
    }

    const std::shared_ptr<MeteoriteDirector> director =
        director_.lock();

    const bool restored =
        player->IsResultChain();

    const bool meteoriteFinished =
        !director || director->IsFinished();

    resultFixed_ =
        restored &&
        meteoriteFinished;

    // 結果が確定するまではPlanet判定を確定させない。
    if (!resultFixed_) {
        planetTouched_ = { false, false };
        isClear_ = false;
        return;
    }

    for (size_t i = 0; i < planets_.size(); ++i) {

        if (!planets_[i]) {
            planetTouched_[i] = false;
            continue;
        }

        const CalyxEngine::Vector3 planetCenter =
            planets_[i]->GetWorldTransform().GetWorldPosition();

        planetTouched_[i] =
            player->IsConnectedFloaterHandInsideRadius(
                planetCenter,
                planetRadius_
            );
    }

    // 左右両方のPlanetにFloaterの手が入っていた時だけCLEAR。
    isClear_ =
        planetTouched_[0] &&
        planetTouched_[1];

    // 現在の最上位Stageを初クリアした時だけ、次のStageを解放する。
    // 一度++されると stageIndex != stageClearCount になるので連続加算されない。
    if (!addIndex_ && isClear_ && ResultCarry::stageIndex == ResultCarry::stageClearCount) {
		addIndex_ = true;
        ++ResultCarry::stageClearCount;
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

    for (size_t i = 0; i < planets_.size(); ++i) {

        planets_[i] = SceneAPI::Instantiate<Planet>();
        if (!planets_[i]) {
            continue;
        }
        planets_[i]->Initialize();
        auto& wt = planets_[i]->GetWorldTransform();
        float posX = static_cast<float>(i * 2);
        wt.translation = { (posX - 1.0f) * 29.0f, 0.0f, 0.0f };
        wt.rotationSource = RotationSource::Euler;
        wt.eulerRotation.x = std::numbers::pi_v<float> *0.5f;
        wt.scale = { 15.0f, 15.0f, 15.0f };
    }
}
