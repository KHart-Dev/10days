#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <array>
#include <memory>

class ResultManager;
class UiSprite;

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

    enum class Selection {
        Title = 0,
        Secondary = 1
    };

    void FindResultManager();

    void InitializeMenuUi();
    void UpdateMenuUi();
    void UpdateInput();

    void ApplySelectionVisual();
    void ConfirmSelection();

    bool CanShowMenu() const;
    bool IsClearResult() const;
    bool CanGoToNextStage() const;
    bool CanSelectSecondary() const;

    void DisableGravity();

private:

    std::weak_ptr<ResultManager> resultManager_;

    // [0] = タイトルへ
    // [1] = CLEAR時: 次のステージへ
    //       GAMEOVER時: もう一度
    std::array<std::shared_ptr<UiSprite>, 2> menuSprites_{};

    Selection selection_ = Selection::Secondary;

    // 左スティックを倒しっぱなしにした時の連続入力防止
    bool stickReady_ = true;

    // シーン遷移要求の連打防止
    bool transitionRequested_ = false;

    // Stage0～4
    static constexpr int kLastStageIndex_ = 4;

    // UI調整値
    float leftX_ = 470.0f;
    float rightX_ = 810.0f;
    float centerY_ = 430.0f;

    float width_ = 240.0f;
    float height_ = 90.0f;
    float selectedScale_ = 1.15f;

    int orderInLayer_ = 1200;
};
