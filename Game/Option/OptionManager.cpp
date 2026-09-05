
#include "OptionManager.h"
// engine
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"

// game
#include <Game/UI/UiSprite.h>

// std
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace {

    // 実際のTexture名に合わせて変更してください。
    constexpr const char* kOptionPanelTexture =
        "Textures/Option/option.png";

    constexpr const char* kLeftTopTexture =
        "Textures/Option/option_left_top.png";
    constexpr const char* kLeftBottomTexture =
        "Textures/Option/option_left_bottom.png";
    constexpr const char* kRightTopTexture =
        "Textures/Option/option_right_top.png";
    constexpr const char* kRightBottomTexture =
        "Textures/Option/option_right_bottom.png";

    // 左向き三角。右側は同じTextureを180度回転して使用する。
    constexpr const char* kOptionArrowTexture =
        "Textures/Option/optionChoice.png";
}

OptionManager::OptionManager()
    : Actor("debugCube.obj", "OptionManager") {

    // Player と同じように保存済みパラメータを読み込む
    param_.LoadParams();
}

OptionManager::~OptionManager() {
    // Optionを開いたままシーンを抜けても次のシーンが停止したままにならないようにする。
    ResumeGameTime();
}

void OptionManager::Initialize() {

    Actor::Initialize();

    DisableGravity();
    Actor::SetDrawEnable(false);

    InitializeSprites();
    SetAllVisible(false);

    animationState_ = AnimationState::Closed;
    animationTimer_ = 0.0f;
    leftArrowFeedbackTimer_ = 0.0f;
    rightArrowFeedbackTimer_ = 0.0f;
    isOpen_ = false;
    gameTimePaused_ = false;

    lastUiUpdateTime_ = std::chrono::steady_clock::now();
    uiClockInitialized_ = true;
}

void OptionManager::Update(float dt) {

    // ClockManagerのTimeScaleが0になるとdtも0になるため、
    // OptionのUIアニメーションだけは実時間ベースのDeltaTimeで更新する。
    const float uiDt = GetUiDeltaTime();

    // 引数dtはゲーム時間。Option表示中は0になる想定なのでUIアニメーションには使わない。
    (void)dt;

    // Inspectorで値を変更したときに、サイズ・角度・配置をその場で確認できるようにする。
    SetupCornerMotions();
    ApplyStaticSpriteParams();

    UpdateInput();
    UpdateArrowFeedback(uiDt);
    UpdateAnimation(uiDt);

    // Opened中はパラメータ変更や押下色を毎フレーム反映する。
    if (animationState_ == AnimationState::Opened) {
        ApplyAnimation(1.0f);
    }
}

void OptionManager::Open() {

    if (animationState_ == AnimationState::Opened ||
        animationState_ == AnimationState::Opening) {
        return;
    }

    SetupCornerMotions();
    ApplyStaticSpriteParams();

    isOpen_ = true;
    animationState_ = AnimationState::Opening;
    animationTimer_ = 0.0f;
    leftArrowFeedbackTimer_ = 0.0f;
    rightArrowFeedbackTimer_ = 0.0f;

    SetAllVisible(true);
    ApplyAnimation(0.0f);

    // Player / Floater / ギミックなどゲーム側のDeltaTimeを止める。
    // UIはGetUiDeltaTime()で実時間更新するので、Optionアニメーションは止まらない。
    PauseGameTime();
}

void OptionManager::Close() {

    if (animationState_ == AnimationState::Closed ||
        animationState_ == AnimationState::Closing) {
        return;
    }

    animationState_ = AnimationState::Closing;
    animationTimer_ = 0.0f;
}

void OptionManager::Toggle() {

    if (isOpen_) {
        Close();
    } else {
        Open();
    }
}

void OptionManager::InitializeSprites() {

    // ==============================
    // 中央パネル
    // ==============================
    optionPanel_ = SceneAPI::Instantiate<UiSprite>(kOptionPanelTexture);
    if (optionPanel_) {
        optionPanel_->SetAnchor({ 0.5f, 0.5f });
        optionPanel_->SetPositionPx(
            param_.panelCenterX,
            param_.panelCenterY
        );
    }

    // ==============================
    // 中央パネル内の左右ピンク三角
    // ==============================
    leftArrow_ = SceneAPI::Instantiate<UiSprite>(kOptionArrowTexture);
    rightArrow_ = SceneAPI::Instantiate<UiSprite>(kOptionArrowTexture);

    if (leftArrow_) {
        leftArrow_->SetAnchor({ 0.5f, 0.5f });
        leftArrow_->SetRotationDeg(0.0f);
    }

    if (rightArrow_) {
        rightArrow_->SetAnchor({ 0.5f, 0.5f });
        rightArrow_->SetRotationDeg(180.0f);
    }

    // ==============================
    // 4隅Sprite
    // ==============================
    corners_[static_cast<size_t>(CornerIndex::LeftTop)].sprite =
        SceneAPI::Instantiate<UiSprite>(kLeftTopTexture);

    corners_[static_cast<size_t>(CornerIndex::LeftBottom)].sprite =
        SceneAPI::Instantiate<UiSprite>(kLeftBottomTexture);

    corners_[static_cast<size_t>(CornerIndex::RightTop)].sprite =
        SceneAPI::Instantiate<UiSprite>(kRightTopTexture);

    corners_[static_cast<size_t>(CornerIndex::RightBottom)].sprite =
        SceneAPI::Instantiate<UiSprite>(kRightBottomTexture);

    for (SpriteMotion& motion : corners_) {
        if (!motion.sprite) {
            continue;
        }

        motion.sprite->SetAnchor({ 0.5f, 0.5f });
    }

    SetupCornerMotions();
    ApplyStaticSpriteParams();

    for (SpriteMotion& motion : corners_) {
        if (!motion.sprite) {
            continue;
        }
        motion.sprite->SetPositionPx(motion.startX, motion.startY);
    }
}

void OptionManager::SetupCornerMotions() {

    const float insetX = param_.cornerInsetX;
    const float insetY = param_.cornerInsetY;
    const float outsideX = param_.cornerStartOutsideX;
    const float outsideY = param_.cornerStartOutsideY;
    const float angle = param_.cornerRotationDeg;

    // 左上
    {
        SpriteMotion& motion =
            corners_[static_cast<size_t>(CornerIndex::LeftTop)];

        motion.startX = -outsideX;
        motion.startY = -outsideY;
        motion.targetX = insetX;
        motion.targetY = insetY;

        // 中央へ向くように斜めにする
        motion.rotationDeg = angle;
    }

    // 左下
    {
        SpriteMotion& motion =
            corners_[static_cast<size_t>(CornerIndex::LeftBottom)];

        motion.startX = -outsideX;
        motion.startY = kScreenHeight + outsideY;
        motion.targetX = insetX;
        motion.targetY = kScreenHeight - insetY;

        motion.rotationDeg = -angle;
    }

    // 右上
    {
        SpriteMotion& motion =
            corners_[static_cast<size_t>(CornerIndex::RightTop)];

        motion.startX = kScreenWidth + outsideX;
        motion.startY = -outsideY;
        motion.targetX = kScreenWidth - insetX;
        motion.targetY = insetY;

        motion.rotationDeg = -angle;
    }

    // 右下
    {
        SpriteMotion& motion =
            corners_[static_cast<size_t>(CornerIndex::RightBottom)];

        motion.startX = kScreenWidth + outsideX;
        motion.startY = kScreenHeight + outsideY;
        motion.targetX = kScreenWidth - insetX;
        motion.targetY = kScreenHeight - insetY;

        motion.rotationDeg = angle;
    }
}

void OptionManager::ApplyStaticSpriteParams() {

    if (optionPanel_) {
        optionPanel_->SetSizePx(param_.panelWidth, param_.panelHeight);
        optionPanel_->SetOrderInLayer(param_.panelOrderInLayer);
    }

    if (leftArrow_) {
        leftArrow_->SetSizePx(param_.arrowWidth, param_.arrowHeight);
        leftArrow_->SetRotationDeg(0.0f);
        leftArrow_->SetOrderInLayer(param_.arrowOrderInLayer);
    }

    if (rightArrow_) {
        rightArrow_->SetSizePx(param_.arrowWidth, param_.arrowHeight);
        rightArrow_->SetRotationDeg(180.0f);
        rightArrow_->SetOrderInLayer(param_.arrowOrderInLayer);
    }

    for (SpriteMotion& motion : corners_) {
        if (!motion.sprite) {
            continue;
        }

        motion.sprite->SetSizePx(
            param_.cornerWidth,
            param_.cornerHeight
        );

        motion.sprite->SetOrderInLayer(param_.cornerOrderInLayer);

        // UiSpriteへ追加したZ回転用Setter。
        motion.sprite->SetRotationDeg(motion.rotationDeg);
    }
}

void OptionManager::UpdateInput() {

    // Esc : 開く / 閉じる
    if (CalyxFoundation::Input::TriggerKey(DIK_ESCAPE)) {
        Toggle();
        return;
    }

    // Start(Menu) : 開く
    if (!isOpen_) {
        if (CalyxFoundation::Input::TriggerGamepadButton(
            CalyxFoundation::PadButton::START)) {
            Open();
        }
        return;
    }

    // B : 閉じる
    if (CalyxFoundation::Input::TriggerGamepadButton(
        CalyxFoundation::PadButton::B)) {
        Close();
        return;
    }

    if (animationState_ != AnimationState::Opened) {
        return;
    }

    // Space / A : 決定
    if (CalyxFoundation::Input::TriggerKey(DIK_SPACE) ||
        CalyxFoundation::Input::TriggerGamepadButton(
            CalyxFoundation::PadButton::A)) {
        ConfirmSelection();
    }

    // ← → : 選択
    if (CalyxFoundation::Input::TriggerKey(DIK_LEFT)) {
        MoveSelection(-1);
    }

    if (CalyxFoundation::Input::TriggerKey(DIK_RIGHT)) {
        MoveSelection(1);
    }

    const CalyxEngine::Vector2 leftStick =
        CalyxFoundation::Input::GetInstance()->GetLeftStick();

    const float stickThreshold =
        std::clamp(param_.stickThreshold, 0.0f, 1.0f);

    const float releaseThreshold =
        std::clamp(
            param_.stickReleaseThreshold,
            0.0f,
            stickThreshold
        );

    if (stickReady_) {
        if (leftStick.x <= -stickThreshold) {
            MoveSelection(-1);
            stickReady_ = false;
        } else if (leftStick.x >= stickThreshold) {
            MoveSelection(1);
            stickReady_ = false;
        }
    } else if (std::abs(leftStick.x) <= releaseThreshold) {
        stickReady_ = true;
    }
}

void OptionManager::UpdateArrowFeedback(float dt) {

    leftArrowFeedbackTimer_ =
        std::fmax(0.0f, leftArrowFeedbackTimer_ - dt);
    rightArrowFeedbackTimer_ =
        std::fmax(0.0f, rightArrowFeedbackTimer_ - dt);
}

void OptionManager::TriggerArrowFeedback(int direction) {

    const float duration = std::fmax(param_.arrowPressedDuration, 0.0f);

    if (direction < 0) {
        leftArrowFeedbackTimer_ = duration;
    } else if (direction > 0) {
        rightArrowFeedbackTimer_ = duration;
    }
}

void OptionManager::UpdateAnimation(float dt) {

    switch (animationState_) {
    case AnimationState::Closed:
    case AnimationState::Opened:
        return;

    case AnimationState::Opening: {
        animationTimer_ += dt;

        const float duration = std::fmax(param_.openDuration, 0.001f);
        const float t = std::clamp(animationTimer_ / duration, 0.0f, 1.0f);

        ApplyAnimation(EaseOutBack(t));

        if (t >= 1.0f) {
            ApplyAnimation(1.0f);
            animationState_ = AnimationState::Opened;
        }
        break;
    }

    case AnimationState::Closing: {
        animationTimer_ += dt;

        const float duration = std::fmax(param_.closeDuration, 0.001f);
        const float t = std::clamp(animationTimer_ / duration, 0.0f, 1.0f);

        ApplyAnimation(1.0f - EaseInBack(t));

        if (t >= 1.0f) {
            ApplyAnimation(0.0f);
            SetAllVisible(false);

            animationState_ = AnimationState::Closed;
            isOpen_ = false;

            // 閉じるアニメーションが最後まで終わってからゲームを再開する。
            ResumeGameTime();
        }
        break;
    }
    }
}

void OptionManager::SetAllVisible(bool visible) {

    if (optionPanel_) {
        optionPanel_->SetVisible(visible);
    }

    if (leftArrow_) {
        leftArrow_->SetVisible(visible);
    }

    if (rightArrow_) {
        rightArrow_->SetVisible(visible);
    }

    for (SpriteMotion& motion : corners_) {
        if (motion.sprite) {
            motion.sprite->SetVisible(visible);
        }
    }
}

void OptionManager::ApplyAnimation(float t) {

    // EaseOutBackではtが1を少し超えるため、最後に少し行き過ぎて戻る。
    for (SpriteMotion& motion : corners_) {
        if (!motion.sprite) {
            continue;
        }

        const float x = Lerp(motion.startX, motion.targetX, t);
        const float y = Lerp(motion.startY, motion.targetY, t);

        motion.sprite->SetPositionPx(x, y);
    }

    const float panelT = std::clamp(t, 0.0f, 1.0f);
    const float startScale = std::fmax(param_.panelStartScale, 0.0f);
    const float scale = Lerp(startScale, 1.0f, panelT);

    if (optionPanel_) {
        optionPanel_->SetPositionPx(
            param_.panelCenterX,
            param_.panelCenterY
        );

        optionPanel_->SetSizePx(
            param_.panelWidth * scale,
            param_.panelHeight * scale
        );

        optionPanel_->SetColorRGBA(
            1.0f,
            1.0f,
            1.0f,
            panelT
        );
    }

    // 三角も中央パネルと一緒に拡大・フェードさせる。
    const float pressedBrightness =
        std::clamp(param_.arrowPressedBrightness, 0.0f, 1.0f);

    if (leftArrow_) {
        const float brightness =
            leftArrowFeedbackTimer_ > 0.0f ? pressedBrightness : 1.0f;

        leftArrow_->SetPositionPx(
            param_.panelCenterX - param_.arrowOffsetX * scale,
            param_.panelCenterY + param_.arrowOffsetY * scale
        );
        leftArrow_->SetSizePx(
            param_.arrowWidth * scale,
            param_.arrowHeight * scale
        );
        leftArrow_->SetColorRGBA(
            brightness, brightness, brightness, panelT
        );
    }

    if (rightArrow_) {
        const float brightness =
            rightArrowFeedbackTimer_ > 0.0f ? pressedBrightness : 1.0f;

        rightArrow_->SetPositionPx(
            param_.panelCenterX + param_.arrowOffsetX * scale,
            param_.panelCenterY + param_.arrowOffsetY * scale
        );
        rightArrow_->SetSizePx(
            param_.arrowWidth * scale,
            param_.arrowHeight * scale
        );
        rightArrow_->SetColorRGBA(
            brightness, brightness, brightness, panelT
        );
    }
}

void OptionManager::MoveSelection(int direction) {

    TriggerArrowFeedback(direction);

    if (optionCount_ <= 0) {
        selectedIndex_ = 0;
        return;
    }

    selectedIndex_ += direction;

    if (selectedIndex_ < 0) {
        selectedIndex_ = optionCount_ - 1;
    } else if (selectedIndex_ >= optionCount_) {
        selectedIndex_ = 0;
    }

    // TODO: 選択カーソルや画像差し替え
}

void OptionManager::ConfirmSelection() {

    // TODO:
    // 0 : やりなおす
    // 1 : タイトルへ戻る
}

void OptionManager::DisableGravity() {

    auto& movement = GetCharacterMovement();
    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);
}

void OptionManager::PauseGameTime() {

    if (gameTimePaused_) {
        return;
    }

    ClockManager::GetInstance()->SetTimeScale(0.0f);
    gameTimePaused_ = true;
}

void OptionManager::ResumeGameTime() {

    if (!gameTimePaused_) {
        return;
    }

    // 通常ゲーム速度へ戻す。
    ClockManager::GetInstance()->SetTimeScale(1.0f);
    gameTimePaused_ = false;
}

float OptionManager::GetUiDeltaTime() {

    const auto now = std::chrono::steady_clock::now();

    if (!uiClockInitialized_) {
        lastUiUpdateTime_ = now;
        uiClockInitialized_ = true;
        return 0.0f;
    }

    const float realDt =
        std::chrono::duration<float>(now - lastUiUpdateTime_).count();

    lastUiUpdateTime_ = now;

    // ブレークポイントやウィンドウ移動後に一気にアニメーションが飛ばないよう上限を設ける。
    return std::clamp(realDt, 0.0f, 0.1f);
}

float OptionManager::EaseOutBack(float t) const {

    const float c1 = std::fmax(param_.backOvershoot, 0.0f);
    const float c3 = c1 + 1.0f;

    const float x = t - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

float OptionManager::EaseInBack(float t) const {

    const float c1 = std::fmax(param_.backOvershoot, 0.0f);
    const float c3 = c1 + 1.0f;

    return c3 * t * t * t - c1 * t * t;
}

float OptionManager::Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void OptionManager::DerivativeGui() {

    using namespace GuiCmd;

    if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
        PropertyText("State", "%s", isOpen_ ? "Open" : "Closed");
        PropertyText("Selected", "%d", selectedIndex_);

        // Player と同じようにSerializableObjectの項目をInspectorへ表示
        param_.ShowGui();

        EndSection();
    }
}

void OptionManager::ApplyConfigFromJson(const nlohmann::json& j) {

    Actor::ApplyConfigFromJson(j);

    const std::string typeKey(GetTypeName());
    const nlohmann::json* src = &j;

    if (j.contains(typeKey)) {
        src = &j.at(typeKey);
    }

    param_.cornerWidth =
        src->value("cornerWidth", param_.cornerWidth);
    param_.cornerHeight =
        src->value("cornerHeight", param_.cornerHeight);
    param_.cornerRotationDeg =
        src->value("cornerRotationDeg", param_.cornerRotationDeg);
    param_.cornerInsetX =
        src->value("cornerInsetX", param_.cornerInsetX);
    param_.cornerInsetY =
        src->value("cornerInsetY", param_.cornerInsetY);
    param_.cornerStartOutsideX =
        src->value("cornerStartOutsideX", param_.cornerStartOutsideX);
    param_.cornerStartOutsideY =
        src->value("cornerStartOutsideY", param_.cornerStartOutsideY);

    param_.openDuration =
        src->value("openDuration", param_.openDuration);
    param_.closeDuration =
        src->value("closeDuration", param_.closeDuration);
    param_.backOvershoot =
        src->value("backOvershoot", param_.backOvershoot);

    param_.panelWidth =
        src->value("panelWidth", param_.panelWidth);
    param_.panelHeight =
        src->value("panelHeight", param_.panelHeight);
    param_.panelCenterX =
        src->value("panelCenterX", param_.panelCenterX);
    param_.panelCenterY =
        src->value("panelCenterY", param_.panelCenterY);
    param_.panelStartScale =
        src->value("panelStartScale", param_.panelStartScale);

    param_.cornerOrderInLayer =
        src->value("cornerOrderInLayer", param_.cornerOrderInLayer);
    param_.panelOrderInLayer =
        src->value("panelOrderInLayer", param_.panelOrderInLayer);
    param_.arrowOrderInLayer =
        src->value("arrowOrderInLayer", param_.arrowOrderInLayer);

    param_.arrowWidth =
        src->value("arrowWidth", param_.arrowWidth);
    param_.arrowHeight =
        src->value("arrowHeight", param_.arrowHeight);
    param_.arrowOffsetX =
        src->value("arrowOffsetX", param_.arrowOffsetX);
    param_.arrowOffsetY =
        src->value("arrowOffsetY", param_.arrowOffsetY);
    param_.arrowPressedBrightness =
        src->value("arrowPressedBrightness", param_.arrowPressedBrightness);
    param_.arrowPressedDuration =
        src->value("arrowPressedDuration", param_.arrowPressedDuration);

    param_.stickThreshold =
        src->value("stickThreshold", param_.stickThreshold);
    param_.stickReleaseThreshold =
        src->value("stickReleaseThreshold", param_.stickReleaseThreshold);

    // 既にSpriteが生成済みならその場で反映する
    SetupCornerMotions();
    ApplyStaticSpriteParams();
}

void OptionManager::ExtractConfigToJson(nlohmann::json& j) const {

    Actor::ExtractConfigToJson(j);

    const std::string typeKey(GetTypeName());
    nlohmann::json derived;

    derived["cornerWidth"] = param_.cornerWidth;
    derived["cornerHeight"] = param_.cornerHeight;
    derived["cornerRotationDeg"] = param_.cornerRotationDeg;
    derived["cornerInsetX"] = param_.cornerInsetX;
    derived["cornerInsetY"] = param_.cornerInsetY;
    derived["cornerStartOutsideX"] = param_.cornerStartOutsideX;
    derived["cornerStartOutsideY"] = param_.cornerStartOutsideY;

    derived["openDuration"] = param_.openDuration;
    derived["closeDuration"] = param_.closeDuration;
    derived["backOvershoot"] = param_.backOvershoot;

    derived["panelWidth"] = param_.panelWidth;
    derived["panelHeight"] = param_.panelHeight;
    derived["panelCenterX"] = param_.panelCenterX;
    derived["panelCenterY"] = param_.panelCenterY;
    derived["panelStartScale"] = param_.panelStartScale;

    derived["cornerOrderInLayer"] = param_.cornerOrderInLayer;
    derived["panelOrderInLayer"] = param_.panelOrderInLayer;
    derived["arrowOrderInLayer"] = param_.arrowOrderInLayer;

    derived["arrowWidth"] = param_.arrowWidth;
    derived["arrowHeight"] = param_.arrowHeight;
    derived["arrowOffsetX"] = param_.arrowOffsetX;
    derived["arrowOffsetY"] = param_.arrowOffsetY;
    derived["arrowPressedBrightness"] = param_.arrowPressedBrightness;
    derived["arrowPressedDuration"] = param_.arrowPressedDuration;

    derived["stickThreshold"] = param_.stickThreshold;
    derived["stickReleaseThreshold"] = param_.stickReleaseThreshold;

    j[typeKey] = std::move(derived);
}