#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Application/Effects/EffectPlayer.h>

/// <summary>ゲーム中に xz 面を流れる隕石（流れ星）</summary>
CALYX_OBJECT(Category = GameObject, DisplayName = "Meteorite", Icon = "UI/Tool/cube.dds")
class Meteorite : public Actor {

public:

	Meteorite();
	// 消えるときに必ずエフェクトを止めたいので、既定にせず自前で持つ
	~Meteorite() override;

	void Initialize() override;
	void Update(float dt) override;

	/// <param name="spinSpeed">y 軸まわりに回る角速度 (rad/s)。符号で向きが変わる</param>
	void Launch(const CalyxEngine::Vector3& velocity, float colliderRadius, float spinSpeed);

	/// この円から出たら自分で Destroy する
	void SetBounds(const CalyxEngine::Vector3& center, float radius);

	/// 尾のエフェクトを止める。二重に呼んでも安全
	void StopEffect();

	bool IsDead() const { return dead_; }
	void MarkDead() { dead_ = true; }

private:

	void SetupCollider(float radius);
	void DisableGravity();
	bool IsOutOfBounds() const;

	CalyxEngine::Vector3 velocity_{};
	float spinSpeed_ = 0.0f;

	CalyxEngine::Vector3 boundsCenter_{};
	float boundsRadius_ = 60.0f;

	bool dead_ = false;

	CalyxEngine::EffectAsset moveEffect_;
	CalyxEngine::EffectHandle moveHandle_{};
	CalyxEngine::EffectAsset breakEffect_;

};
