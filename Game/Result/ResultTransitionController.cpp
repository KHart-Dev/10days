#include "ResultTransitionController.h"

#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Result/ResultManager.h>
#include <Game/Result/ResultCarry.h>
#include <Game/UI/UiSprite.h>
#include <Game/Scene/SceneFlow.h>

#include <cmath>

namespace {

    constexpr const char* kTitleTexture =
        "Textures/Result/title.png";

    constexpr const char* kNextStageTexture =
        "Textures/Result/nextStage.png";

    constexpr const char* kRetryTexture =
        "Textures/Result/retry.png";
}

ResultTransitionController::ResultTransitionController()
    : Actor("debugCube.obj", "ResultTransitionController") {}

void ResultTransitionController::Initialize() {

    Actor::Initialize();

    DisableGravity();
    SetDrawEnable(false);

    FindResultManager();
    InitializeMenuUi();
}

void ResultTransitionController::Update([[maybe_unused]] float dt) {

    // Sceneの初期化順によってInitialize時に見つからなかった場合にも対応する。
    if (resultManager_.expired()) {
        FindResultManager();
    }

    UpdateMenuUi();
    UpdateInput();
}

void ResultTransitionController::FindResultManager() {

    auto* ctx = SceneContext::Current();

    if (!ctx) {
        resultManager_.reset();
        return;
    }

    resultManager_ =
        ctx->FindFirst<ResultManager>();
}

void ResultTransitionController::InitializeMenuUi() {

    menuSprites_[0] =
        SceneAPI::Instantiate<UiSprite>(
            kTitleTexture
        );

    // 初期状態ではnextStageを設定しておく。
    // 結果確定後、失敗ならretryへ差し替える。
    menuSprites_[1] =
        SceneAPI::Instantiate<UiSprite>(
            kNextStageTexture
        );

    for (std::shared_ptr<UiSprite>& sprite : menuSprites_) {

        if (!sprite) {
            continue;
        }

        sprite->SetAnchor({ 0.5f, 0.5f });
        sprite->SetOrderInLayer(orderInLayer_);
        sprite->SetVisible(false);
    }

    if (menuSprites_[0]) {
        menuSprites_[0]->SetPositionPx(
            leftX_,
            centerY_
        );
    }

    if (menuSprites_[1]) {
        menuSprites_[1]->SetPositionPx(
            rightX_,
            centerY_
        );
    }

    selection_ = Selection::Secondary;
    stickReady_ = true;
    transitionRequested_ = false;

    ApplySelectionVisual();
}

void ResultTransitionController::UpdateMenuUi() {

    const bool showMenu =
        CanShowMenu();

    if (menuSprites_[0]) {
        menuSprites_[0]->SetVisible(showMenu);
    }

    if (!showMenu) {

        if (menuSprites_[1]) {
            menuSprites_[1]->SetVisible(false);
        }

        return;
    }

    const bool isClear =
        IsClearResult();

    // =========================================
    // 右側Spriteを結果によって切り替える
    //
    // CLEAR    : 次のステージ
    // GAMEOVER : もう一度
    // =========================================
    if (menuSprites_[1]) {

        if (isClear) {
            menuSprites_[1]->SetTexture(
                kNextStageTexture
            );
        } else {
            menuSprites_[1]->SetTexture(
                kRetryTexture
            );
        }

        // Stage4をCLEARした場合だけ、
        // 次のStageが無いので右側を非表示にする。
        menuSprites_[1]->SetVisible(
            CanSelectSecondary()
        );
    }

    if (menuSprites_[0]) {
        menuSprites_[0]->SetPositionPx(
            leftX_,
            centerY_
        );
    }

    if (menuSprites_[1]) {
        menuSprites_[1]->SetPositionPx(
            rightX_,
            centerY_
        );
    }

    // 右側が選択不可になった場合はTitleへ戻す。
    if (!CanSelectSecondary() &&
        selection_ == Selection::Secondary) {

        selection_ = Selection::Title;
    }

    ApplySelectionVisual();
}

void ResultTransitionController::UpdateInput() {

    if (!CanShowMenu() ||
        transitionRequested_) {

        return;
    }

    bool changed = false;

    // キーボード ←
    if (CalyxFoundation::Input::TriggerKey(DIK_LEFT)) {

        if (selection_ != Selection::Title) {

            selection_ = Selection::Title;
            changed = true;
        }
    }

    // キーボード →
    if (CalyxFoundation::Input::TriggerKey(DIK_RIGHT)) {

        if (CanSelectSecondary() &&
            selection_ != Selection::Secondary) {

            selection_ = Selection::Secondary;
            changed = true;
        }
    }

    // ゲームパッド左スティック
    const CalyxEngine::Vector2 leftStick =
        CalyxFoundation::Input::GetInstance()->GetLeftStick();

    constexpr float kStickThreshold = 0.6f;
    constexpr float kStickReleaseThreshold = 0.3f;

    if (stickReady_) {

        if (leftStick.x <= -kStickThreshold) {

            if (selection_ != Selection::Title) {

                selection_ = Selection::Title;
                changed = true;
            }

            stickReady_ = false;

        } else if (leftStick.x >= kStickThreshold) {

            if (CanSelectSecondary() &&
                selection_ != Selection::Secondary) {

                selection_ = Selection::Secondary;
                changed = true;
            }

            stickReady_ = false;
        }

    } else if (
        std::abs(leftStick.x) <=
        kStickReleaseThreshold) {

        stickReady_ = true;
    }

    if (changed) {
        ApplySelectionVisual();
    }

    // Space / A で決定
    if (CalyxFoundation::Input::TriggerKey(DIK_SPACE) ||
        CalyxFoundation::Input::TriggerGamepadButton(
            CalyxFoundation::PadButton::A)) {

        ConfirmSelection();
    }
}

void ResultTransitionController::ApplySelectionVisual() {

    const float selectedWidth =
        width_ * selectedScale_;

    const float selectedHeight =
        height_ * selectedScale_;

    for (size_t i = 0; i < menuSprites_.size(); ++i) {

        const std::shared_ptr<UiSprite>& sprite =
            menuSprites_[i];

        if (!sprite) {
            continue;
        }

        const bool selected =
            static_cast<int>(selection_) ==
            static_cast<int>(i);

        sprite->SetSizePx(
            selected ? selectedWidth : width_,
            selected ? selectedHeight : height_
        );
    }
}

void ResultTransitionController::ConfirmSelection() {

    if (transitionRequested_) {
        return;
    }

    switch (selection_) {

        // =========================
        // 左 : タイトルへ
        // =========================
    case Selection::Title:

        transitionRequested_ = true;

        SceneFlow::GoToTitle();
        break;

        // =========================
        // 右
        //
        // CLEAR    -> 次のStage
        // GAMEOVER -> 同じStageをもう一度
        // =========================
    case Selection::Secondary:

        if (!CanSelectSecondary()) {
            return;
        }

        transitionRequested_ = true;

        if (IsClearResult()) {

            SceneFlow::StartStage(
                ResultCarry::stageIndex + 1
            );

        } else {

            SceneFlow::StartStage(
                ResultCarry::stageIndex
            );
        }

        break;
    }
}

bool ResultTransitionController::CanShowMenu() const {

    const std::shared_ptr<ResultManager> resultManager =
        resultManager_.lock();

    if (!resultManager) {
        return false;
    }

    // CLEARでもGAMEOVERでも、結果が確定したらメニューを表示する。
    return
        resultManager->IsResultFixed();
}

bool ResultTransitionController::IsClearResult() const {

    const std::shared_ptr<ResultManager> resultManager =
        resultManager_.lock();

    if (!resultManager) {
        return false;
    }

    return
        resultManager->IsClear();
}

bool ResultTransitionController::CanGoToNextStage() const {

    return
        ResultCarry::stageIndex >= 0 &&
        ResultCarry::stageIndex < kLastStageIndex_;
}

bool ResultTransitionController::CanSelectSecondary() const {

    // 失敗時の「もう一度」はどのStageでも使用可能。
    if (!IsClearResult()) {
        return true;
    }

    // CLEAR時だけ、次Stageが存在するか確認する。
    return
        CanGoToNextStage();
}

void ResultTransitionController::DisableGravity() {

    auto& movement =
        GetCharacterMovement();

    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);
}
