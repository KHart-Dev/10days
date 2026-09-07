#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>

#include <array>
#include <memory>
#include <vector>

class Player;
class FloaterManager;
class Floater;
class Planet;
class UiSprite;

CALYX_OBJECT(
    Category = GameObject,
    DisplayName = "ResultManager",
    Icon = "Textures/white1x1.png"
)
class ResultManager : public Actor {

public:

    ResultManager();
    ~ResultManager() override = default;

    void Initialize() override;
    void Update(float dt) override;

    bool IsClear() const { return isClear_; }
    bool IsPlanetTouched(size_t index) const {
        return index < planetTouched_.size() ? planetTouched_[index] : false;
    }

private:

    void InitializeActor();
    void SpawnPlanets();

    void InitializeResultUi();
    void UpdateResultUi();
    void InitializeDistanceUi();
    void UpdateDistanceUi();

    void CreateDistanceSpriteGroup(
        std::array<std::shared_ptr<UiSprite>, 2>& sprites,
        const char* texturePath);

    void LayoutDistanceSpriteGroup(
        const std::array<std::shared_ptr<UiSprite>, 2>& sprites,
        float onesX,
        float centerY);

    void SetDistanceSpriteValue(
        const std::array<std::shared_ptr<UiSprite>, 2>& sprites,
        int value);

    void DisableGravity();

private:

    std::shared_ptr<Player> player_;

    std::shared_ptr<FloaterManager> floaterManager_;

    std::array<std::shared_ptr<Planet>, 2> planets_{};
    std::array<bool, 2> planetTouched_{ false, false };

    // クリア/失敗確認用
    std::shared_ptr<UiSprite> resultColorSprite_;

    // Distance UIは2D UiSpriteで描画する。
    // 上段: 目標距離 / 下段: 実際の人間橋距離
    std::array<std::shared_ptr<UiSprite>, 2> targetDistanceSprites_{};
    std::array<std::shared_ptr<UiSprite>, 2> bridgeDistanceSprites_{};

    // 1280x720基準の画面座標
    // 1の位は必ずX=640に置き、10の位はその左へ配置する。
    float targetDistanceOnesX_ = 640.0f;
    float targetDistanceCenterY_ = 125.0f;
    float bridgeDistanceOnesX_ = 640.0f;
    float bridgeDistanceCenterY_ = 610.0f;

    float distanceDigitWidth_ = 54.0f;
    float distanceDigitHeight_ = 108.0f;
    float distanceDigitSpacing_ = 58.0f;
    int distanceUiOrderInLayer_ = 1100;

    size_t nextFloaterIndex_ = 1;

    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 0.2f;

    float planetRadius_ = 30.0f;

    bool initialized_ = false;
    bool isClear_ = false;
};
