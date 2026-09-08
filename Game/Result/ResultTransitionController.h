#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <memory>

class ResultManager;

CALYX_OBJECT(
    Category = GameObject,
    DisplayName = "ResultTransitionController",
    Icon = "Textures/white1x1.png"
)
class ResultTransitionController : public Actor {

public:

    ResultTransitionController();
    ~ResultTransitionController() override = default;

    void Initialize() override;
    void Update(float dt) override;

private:

    void FindResultManager();

    void UpdateAutoTransition(float dt);
    void TransitionScene();

    bool CanGoToNextStage() const;

    void DisableGravity();

private:

    std::weak_ptr<ResultManager> resultManager_;

    // 結果が確定したか
    bool resultDetected_ = false;

    // 確定した結果を保存
    bool resultIsClear_ = false;

    // 結果表示後の経過時間
    float transitionTimer_ = 0.0f;

    // CLEAR / GAMEOVER表示から遷移までの時間
    float transitionDelay_ = 3.0f;

    // シーン遷移の二重実行防止
    bool transitionRequested_ = false;

    // Stage0 ～ Stage4
    static constexpr int kLastStageIndex_ = 5;
};