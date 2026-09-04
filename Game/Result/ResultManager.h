#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

#include <memory>
#include <vector>

class Player;
class Floater;

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

private:

    void InitializeActor();
    void SpawnNextFloater();
    void DisableGravity();

private:

    std::shared_ptr<Player> player_;

    std::vector<std::shared_ptr<Floater>> resultFloaters_;

    size_t nextFloaterIndex_ = 1;

    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 0.2f;

    bool initialized_ = false;
};