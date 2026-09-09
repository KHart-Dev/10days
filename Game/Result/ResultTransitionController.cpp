#include "ResultTransitionController.h"

#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/Result/ResultManager.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Scene/SceneFlow.h>

ResultTransitionController::ResultTransitionController()
    : Actor("debugCube.obj", "ResultTransitionController") {}

void ResultTransitionController::Initialize() {

    Actor::Initialize();

    DisableGravity();
    SetDrawEnable(false);

    FindResultManager();

    resultDetected_ = false;
    resultIsClear_ = false;
    transitionTimer_ = 0.0f;
    transitionRequested_ = false;
}

void ResultTransitionController::Update(float dt) {

    // Sceneの初期化順によって
    // Initialize時にResultManagerが見つからない場合にも対応
    if (resultManager_.expired()) {
        FindResultManager();
    }

    UpdateAutoTransition(dt);
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

void ResultTransitionController::UpdateAutoTransition(float dt) {

    if (transitionRequested_) {
        return;
    }

    const std::shared_ptr<ResultManager> resultManager =
        resultManager_.lock();

    if (!resultManager) {
        return;
    }

    // =========================================
    // まだCLEAR / GAMEOVERが確定していない
    // =========================================
    if (!resultManager->IsResultFixed()) {

        resultDetected_ = false;
        transitionTimer_ = 0.0f;

        return;
    }

    // =========================================
    // CLEAR / GAMEOVERが確定した瞬間
    // =========================================
    if (!resultDetected_) {

        resultDetected_ = true;

        // この瞬間の結果を保存しておく
        resultIsClear_ =
            resultManager->IsClear();

        transitionTimer_ = 0.0f;
    }

    // =========================================
    // 結果表示後の時間を計測
    // =========================================
    transitionTimer_ += dt;

    if (transitionTimer_ < transitionDelay_) {
        return;
    }

    // =========================================
    // 1秒経過したので自動遷移
    // =========================================
    TransitionScene();
}

void ResultTransitionController::TransitionScene() {

    if (transitionRequested_) {
        return;
    }

    transitionRequested_ = true;

    // =========================================
    // CLEAR
    // =========================================
    if (resultIsClear_) {

        // 次のStageが存在する
        if (CanGoToNextStage()) {

            SceneFlow::StartStage(
                ResultCarry::stageIndex + 1
            );

            return;
        }

        // =====================================
        // 最終Stageをクリアした場合
        //
        // Stage5へ行かないようにTitleへ戻す
        // =====================================
        SceneFlow::GoToAnimationFinal();

        return;
    }

    // =========================================
    // GAMEOVER
    //
    // 現在のStageをもう一度読み込む
    // =========================================
    SceneFlow::StartStage(
        ResultCarry::stageIndex
    );
}

bool ResultTransitionController::CanGoToNextStage() const {

    return
        ResultCarry::stageIndex >= 0 &&
        ResultCarry::stageIndex < kLastStageIndex_;
}

void ResultTransitionController::DisableGravity() {

    auto& movement =
        GetCharacterMovement();

    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);
}