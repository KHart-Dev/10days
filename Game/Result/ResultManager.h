#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <array>
#include <memory>
#include <vector>

class Player;
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

    /// 両方のPlanetにresultFloaters_の手が入っていればtrue
    bool IsClear() const { return isClear_; }

    /// デバッグ/UI確認用
    bool IsPlanetTouched(size_t index) const {
        return index < planetTouched_.size() ? planetTouched_[index] : false;
    }

private:

    void InitializeActor();
    void SpawnNextFloater();
    void SpawnPlanets();
    void CheckStageClear();
    void InitializeResultUi();
    void UpdateResultUi();
    void DisableGravity();

private:

    std::shared_ptr<Player> player_;

    std::vector<std::shared_ptr<Floater>> resultFloaters_;

    // ResultManagerから生成する左右2つのPlanet
    std::array<std::shared_ptr<Planet>, 2> planets_{};

    // 各Planetに、いずれかのFloaterの手が入っているか
    std::array<bool, 2> planetTouched_{ false, false };

    // クリア/失敗を色で表示するUiSprite
    // Clear = 赤 / Failed = 青
    std::shared_ptr<UiSprite> resultColorSprite_;

    size_t nextFloaterIndex_ = 1;

    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 0.2f;

    // Planetの判定半径。
    // 見た目の大きさと判定をPlanet::SetRadius()で合わせる。
    float planetRadius_ = 30.0f;

    bool initialized_ = false;
    bool isClear_ = false;
};
