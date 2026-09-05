#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// std
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>

class UiSprite;

CALYX_OBJECT(
    Category = GameObject,
    DisplayName = "OptionManager",
    Icon = "Textures/white1x1.png"
)
class OptionManager : public Actor {

public:

    OptionManager();
    ~OptionManager() override;

    void Initialize() override;
    void Update(float dt) override;

    void Open();
    void Close();
    void Toggle();

    bool IsOpen() const { return isOpen_; }
    bool IsAnimating() const {
        return animationState_ != AnimationState::Closed &&
            animationState_ != AnimationState::Opened;
    }

    int GetSelectedIndex() const { return selectedIndex_; }

    // シリアライズ / Inspector
    void ApplyConfigFromJson(const nlohmann::json& j) override;
    void ExtractConfigToJson(nlohmann::json& j) const override;
    void DerivativeGui() override;

private:

    enum class AnimationState {
        Closed,
        Opening,
        Opened,
        Closing,
    };

    enum class CornerIndex : size_t {
        LeftTop = 0,
        LeftBottom,
        RightTop,
        RightBottom,
        Count,
    };

    struct SpriteMotion {
        std::shared_ptr<UiSprite> sprite;

        float startX = 0.0f;
        float startY = 0.0f;
        float targetX = 0.0f;
        float targetY = 0.0f;

        float rotationDeg = 0.0f;
    };

    // PlayerParam と同じ形式で調整値を保存する
    struct OptionParam : CalyxEngine::SerializableObject {
        OptionParam() {
            AddField("cornerWidth", cornerWidth)
                .Category("Corner Sprite")
                .Tooltip("4隅から出てくるSpriteの横サイズ(px)");

            AddField("cornerHeight", cornerHeight)
                .Category("Corner Sprite")
                .Tooltip("4隅から出てくるSpriteの縦サイズ(px)");

            AddField("cornerRotationDeg", cornerRotationDeg)
                .Category("Corner Sprite")
                .Tooltip("4隅Spriteの傾き。符号は角ごとに自動反転する(deg)");

            AddField("cornerInsetX", cornerInsetX)
                .Category("Corner Sprite")
                .Tooltip("表示完了時に画面左右端から内側へ入れる距離(px)");

            AddField("cornerInsetY", cornerInsetY)
                .Category("Corner Sprite")
                .Tooltip("表示完了時に画面上下端から内側へ入れる距離(px)");

            AddField("cornerStartOutsideX", cornerStartOutsideX)
                .Category("Corner Sprite")
                .Tooltip("開始位置を画面左右の外へどれだけ離すか(px)");

            AddField("cornerStartOutsideY", cornerStartOutsideY)
                .Category("Corner Sprite")
                .Tooltip("開始位置を画面上下の外へどれだけ離すか(px)");

            AddField("openDuration", openDuration)
                .Category("Animation")
                .Tooltip("オプション画面が出きるまでの時間(sec)");

            AddField("closeDuration", closeDuration)
                .Category("Animation")
                .Tooltip("オプション画面が閉じるまでの時間(sec)");

            AddField("backOvershoot", backOvershoot)
                .Category("Animation")
                .Tooltip("ニュッと少し行き過ぎる強さ。大きいほど勢いが強い");

            AddField("panelWidth", panelWidth)
                .Category("Center Panel")
                .Tooltip("中央オプションパネルの横サイズ(px)");

            AddField("panelHeight", panelHeight)
                .Category("Center Panel")
                .Tooltip("中央オプションパネルの縦サイズ(px)");

            AddField("panelCenterX", panelCenterX)
                .Category("Center Panel")
                .Tooltip("中央オプションパネルの中心X座標(px)");

            AddField("panelCenterY", panelCenterY)
                .Category("Center Panel")
                .Tooltip("中央オプションパネルの中心Y座標(px)");

            AddField("panelStartScale", panelStartScale)
                .Category("Center Panel")
                .Tooltip("中央パネルが出始める時の倍率");

            AddField("cornerOrderInLayer", cornerOrderInLayer)
                .Category("Sorting")
                .Tooltip("4隅SpriteのOrderInLayer");

            AddField("panelOrderInLayer", panelOrderInLayer)
                .Category("Sorting")
                .Tooltip("中央OptionパネルのOrderInLayer");

            AddField("arrowOrderInLayer", arrowOrderInLayer)
                .Category("Sorting")
                .Tooltip("左右矢印SpriteのOrderInLayer。パネルより手前になる値にする");

            AddField("arrowWidth", arrowWidth)
                .Category("Option Arrow")
                .Tooltip("左右のピンク三角Spriteの横サイズ(px)");

            AddField("arrowHeight", arrowHeight)
                .Category("Option Arrow")
                .Tooltip("左右のピンク三角Spriteの縦サイズ(px)");

            AddField("arrowOffsetX", arrowOffsetX)
                .Category("Option Arrow")
                .Tooltip("中央パネル中心から左右の三角までの距離(px)");

            AddField("arrowOffsetY", arrowOffsetY)
                .Category("Option Arrow")
                .Tooltip("中央パネル中心から三角を上下へずらす距離(px)");

            AddField("arrowPressedBrightness", arrowPressedBrightness)
                .Category("Option Arrow")
                .Tooltip("左右入力した瞬間の三角の明るさ。1.0が通常、0.0が黒");

            AddField("arrowPressedDuration", arrowPressedDuration)
                .Category("Option Arrow")
                .Tooltip("左右入力時に三角を暗くしておく時間(sec)");

            AddField("stickThreshold", stickThreshold)
                .Category("Input")
                .Tooltip("左スティックで選択を切り替える入力しきい値");

            AddField("stickReleaseThreshold", stickReleaseThreshold)
                .Category("Input")
                .Tooltip("次のスティック入力を受け付けるための戻ししきい値");
        }

        CalyxEngine::ParamPath GetParamPath() const override {
            return {
                CalyxEngine::ParamDomain::Game,
                "OptionManager",
                "Actor/OptionManager/OptionParam"
            };
        }

        // 4隅Sprite
        float cornerWidth = 300.0f;
        float cornerHeight = 240.0f;
        float cornerRotationDeg = 45.0f;
        float cornerInsetX = 95.0f;
        float cornerInsetY = 90.0f;
        float cornerStartOutsideX = 220.0f;
        float cornerStartOutsideY = 170.0f;

        // アニメーション
        float openDuration = 0.28f;
        float closeDuration = 0.22f;
        float backOvershoot = 1.70158f;

        // 中央パネル
        float panelWidth = 280.0f;
        float panelHeight = 430.0f;
        float panelCenterX = 640.0f;
        float panelCenterY = 360.0f;
        float panelStartScale = 0.86f;

        // Sprite描画順
        // 同一SortingLayer内では、Optionパネルより矢印を手前に描画する。
        int32_t cornerOrderInLayer = 90;
        int32_t panelOrderInLayer = 100;
        int32_t arrowOrderInLayer = 110;

        // 中央パネル内の左右三角
        float arrowWidth = 34.0f;
        float arrowHeight = 46.0f;
        float arrowOffsetX = 82.0f;
        float arrowOffsetY = 0.0f;
        float arrowPressedBrightness = 0.58f;
        float arrowPressedDuration = 0.12f;

        // 入力
        float stickThreshold = 0.55f;
        float stickReleaseThreshold = 0.25f;
    };

private:

    void InitializeSprites();
    void SetupCornerMotions();
    void ApplyStaticSpriteParams();

    void UpdateInput();
    void UpdateAnimation(float dt);
    void UpdateArrowFeedback(float dt);
    void TriggerArrowFeedback(int direction);

    void SetAllVisible(bool visible);
    void ApplyAnimation(float t);

    void MoveSelection(int direction);
    void ConfirmSelection();
    void DisableGravity();

    // Option表示中はゲーム側のDeltaTimeを止める
    void PauseGameTime();
    void ResumeGameTime();

    // TimeScaleが0でもOptionのUIアニメーションだけ動かすための実時間DeltaTime
    float GetUiDeltaTime();

    float EaseOutBack(float t) const;
    float EaseInBack(float t) const;
    static float Lerp(float a, float b, float t);

private:

    OptionParam param_;

    std::shared_ptr<UiSprite> optionPanel_;
    std::shared_ptr<UiSprite> leftArrow_;
    std::shared_ptr<UiSprite> rightArrow_;
    std::array<SpriteMotion, static_cast<size_t>(CornerIndex::Count)> corners_{};

    AnimationState animationState_ = AnimationState::Closed;

    float animationTimer_ = 0.0f;
    float leftArrowFeedbackTimer_ = 0.0f;
    float rightArrowFeedbackTimer_ = 0.0f;

    bool isOpen_ = false;
    bool stickReady_ = true;
    bool gameTimePaused_ = false;

    // Open した時点の TimeScale。Close で必ずここへ戻す
    float scaleBeforePause_ = 1.0f;

    std::chrono::steady_clock::time_point lastUiUpdateTime_{};
    bool uiClockInitialized_ = false;

    int selectedIndex_ = 0;
    int optionCount_ = 2;

    // 1280x720 基準
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
};
