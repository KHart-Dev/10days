#pragma once

#include <Engine/Objects/2D/Object2d/SpriteSceneObject2d.h>
#include <Engine/Foundation/Math/Vector2.h>

#include <algorithm>
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

    /// 数字アトラス等で使用するUVスケール
    void SetUvScale(const CalyxEngine::Vector2& scale) {
        uvScale_ = scale;
        if (sprite_) {
            sprite_->SetUvScale(uvScale_);
        }
    }

    /// 数字アトラス等で使用するUVオフセット
    void SetUvOffset(const CalyxEngine::Vector2& offset) {
        uvOffset_ = offset;
        if (sprite_) {
            sprite_->SetUvOffset(uvOffset_);
        }
    }

    /// 横一列のアトラスから指定フレームを表示
    void SetHorizontalAtlasFrame(int frame, int frameCount) {
        if (frameCount <= 0) {
            return;
        }

        frame = std::clamp(frame, 0, frameCount - 1);

        const float frameWidth =
            1.0f / static_cast<float>(frameCount);

        SetUvScale({ frameWidth, 1.0f });
        SetUvOffset({
            static_cast<float>(frame) * frameWidth,
            0.0f
            });
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

	void SetTexture(const std::string& texturePath) {
		sprite_->SetTexture(texturePath);
	}

private:
    CalyxEngine::Vector2 uvScale_{ 1.0f, 1.0f };
    CalyxEngine::Vector2 uvOffset_{ 0.0f, 0.0f };

    int fillMethod_ = 0;
    float fillOriginX_ = 0.0f;
    float fillOriginY_ = 0.0f;
    float fillAmount_ = 1.0f;
};
