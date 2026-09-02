#include "Thorn.h"

#include <Engine/Objects/Collider/Collider.h>

#include <Game/Player/Player.h>

Thorn::Thorn() :Actor("cone.obj", "thorn") {
	//初期化時にスケールを小さくしておく
	worldTransform_.scale *= 0.5f;
}
Thorn::~Thorn() = default;

// 衝突時処理
void Thorn::OnCollisionEnter(Collider* other){
	BaseGameObject* owner = other ? other->GetOwner() : nullptr;
	if(auto* player = dynamic_cast<Player*>(owner)) {
		player->AllBreak();
	}
}

