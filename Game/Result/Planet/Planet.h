#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>

class Planet : public Actor {

public:

    Planet();
    ~Planet() override = default;

    void Initialize() override;
    void Update(float dt) override;

    /// Planetの見た目サイズと判定半径を設定する
    void SetRadius(float radius);

    float GetRadius() const { return radius_; }

    /// ワールド座標pointがPlanetの球判定内にあるか
    bool ContainsPoint(const CalyxEngine::Vector3& point);

private:

    float radius_ = 1.5f;
};
