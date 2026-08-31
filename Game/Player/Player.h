#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>

// game
#include <Demo/Input/PlayerInput.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

CALYX_OBJECT(Category = GameObject, DisplayName = "Player", Icon = "UI/Tool/cube.dds")
class Player : public Actor {

public:

	Player();
	~Player() override = default;

	void Initialize() override;
	void Update(float dt) override;

private:

	PlayerInput input_;

	// 回転慣性（Y軸）
	float yawVelocity_ = 0.0f;         // 現在の角速度 (rad/s)

	// 保存可能なパラメータは SerializableObject を使ってまとめる
	struct PlayerParam : CalyxEngine::SerializableObject {
		PlayerParam() {
			AddField("moveSpeed", moveSpeed)
				.Category("Movement")
				.Tooltip("移動速度 (m/s)");

			AddField("rotSpeedDeg", rotSpeedDeg)
				.Category("Movement")
				.Tooltip("回転速度 (deg/s)");

			AddField("yawAcceleration", yawAcceleration)
				.Category("Movement")
				.Tooltip("回転慣性の追従係数");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "Player", "Actor/Player/PlayerParam"};
		}

		float moveSpeed = 5.0f;
		float rotSpeedDeg = 180.0f;
		float yawAcceleration = 10.0f;
	};

	PlayerParam param_;

public:
	// シリアライズ用インターフェース
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void DerivativeGui() override;



};

