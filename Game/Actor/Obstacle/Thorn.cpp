#include "Thorn.h"

// game
#include <Game/Collision/CollisionLayerUtil.h>

// engine
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

Thorn::Thorn() :Actor("cone.obj", "thorn") {
	//初期化時にスケールを小さくしておく
	worldTransform_.scale *= 0.5f;
}
Thorn::~Thorn() = default;

// 衝突時処理
void Thorn::OnCollisionEnter(Collider* other){
	if (!other) {
		return;
	}

	const auto playerLayer = GameCollision::FindLayerId("Thon");
		CameraManager::GetMain3d()->StartShake(1.0f, 0.1f);

}

