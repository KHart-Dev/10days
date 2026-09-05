#pragma once

#include <Engine/Objects/2D/Object2d/SpriteSceneObject2d.h>

#include <numbers>
#include <string>

/*-----------------------------------------------------------------------------------------
 * UiSprite
 * - 色/位置/サイズ/回転/表示を動かす汎用2Dスプライト
 *---------------------------------------------------------------------------------------*/
class UiSprite
    : public CalyxEngine::SpriteSceneObject2d {
public:
    UiSprite() = default;

    /// 生成時にテクスチャを指定する(空なら白1x1のまま)
    UiSprite(const std::string& texturePath) {
        if (!texturePath.empty()) {
            texturePath_ = texturePath;
        }
    }

    ~UiSprite() override = default;

    void SetPositionPx(float x, float y) {
        worldTransform_.translation = { x, y, 0.0f };
    }

    void SetSizePx(float w, float h) {
        worldTransform_.scale = { w, h, 1.0f };
    }

    /// 2DなのでZ軸回転。Inspector側では度数法で扱いやすくする。
    void SetRotationDeg(float degree) {
        worldTransform_.eulerRotation.z =
            degree * std::numbers::pi_v<float> / 180.0f;
        worldTransform_.rotationSource = RotationSource::Euler;
    }

    void SetColorRGBA(float r, float g, float b, float a = 1.0f) {
        color_ = { r, g, b, a };
    }

    void SetVisible(bool visible) {
        SetDrawEnable(visible);
    }

    /// 塗りつぶしで見せる範囲を指定する(method: 1=横, 2=縦, 3=マスク / origin: 0=左下, 1=右上)
    void SetFill(int method, float originX, float originY, float amount) {
        fillMethod_ = method;
        fillOriginX_ = originX;
        fillOriginY_ = originY;
        fillAmount_ = amount;
    }

    void AlwaysUpdate(float dt) override {
        CalyxEngine::SpriteSceneObject2d::AlwaysUpdate(dt);

        if (!sprite_) {
            return;
        }

        SetFillMethod(fillMethod_);
        SetFillOrigin(fillOriginX_, fillOriginY_);
        SetFillAmount(fillAmount_);
    }

private:
    int fillMethod_ = 0;
    float fillOriginX_ = 0.0f;
    float fillOriginY_ = 0.0f;
    float fillAmount_ = 1.0f;
};
