#include "Planet.h"

#include <algorithm>

Planet::Planet()
    : Actor("plane.obj", "Planet") {}

void Planet::Initialize() {

    Actor::Initialize();

    // ResultScene上で固定して使うため重力を無効にする。
    auto& movement = GetCharacterMovement();
    movement.SetGravity(0.0f);
    movement.SetMaxFallSpeed(0.0f);
    movement.SetFloorProbeDistance(0.0f);
    movement.SetFloorSnapDistance(0.0f);

    SetRadius(radius_);
    SetTexture("Textures/Planet/Planet.png");
}

void Planet::Update(float dt) {
    Actor::Update(dt);
}

void Planet::SetRadius(float radius) {

    radius_ = std::fmax(radius, 0.0f);

    auto& wt = GetWorldTransform();

    // sphere.objを半径に合わせて拡縮する。
    wt.scale = {
        radius_,
        radius_,
        radius_
    };

    wt.Update();
}

bool Planet::ContainsPoint(
    const CalyxEngine::Vector3& point) {

    const CalyxEngine::Vector3 center =
        GetWorldTransform().GetWorldPosition();

    const CalyxEngine::Vector3 delta =
        point - center;

    return delta.LengthSquared() <= radius_ * radius_;
}