#include "Player.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Input/Input.h>

// std
#include <numbers>

Player::Player()
	: Actor("plane.obj", "Player") {
}

void Player::Initialize() {
	Actor::Initialize();

	auto& wt = GetWorldTransform();
	// 常に X 軸に -90度回転させるため、回転ソースをオイラーにして固定ピッチを設定する
	wt.eulerRotation.x = -std::numbers::pi_v<float> * 0.5f; // pitch = -90deg
	wt.rotationSource = RotationSource::Euler;
	wt.Update();
}

namespace {
	// ワールド座標のXZ平面での入力マッピング。
	// move.x = AD (X軸), move.y = WS (Z軸)
	CalyxEngine::Vector3 BuildWorldMoveDirection(const CalyxEngine::Vector2& move) {
		CalyxEngine::Vector3 direction{move.x, 0.0f, move.y};

		// 斜め入力で速くならないよう正規化
		if (direction.LengthSquared() > 1.0f) {
			direction = direction.Normalize();
		}

		return direction;
	}
}

void Player::Update(float dt) {
	// 入力更新
	input_.Update();
	const PlayerInputState& state = input_.GetState();

	// 移動
	CalyxEngine::Vector3 worldDir = BuildWorldMoveDirection(state.move);
	const float moveSpeed = 5.0f; // m/s
	if(worldDir.LengthSquared() > 0.0f) {
		// 移動量を加算（物理は使わないシンプル実装）
		CalyxEngine::Vector3 delta = worldDir * (moveSpeed * dt);
		auto& wt = GetWorldTransform();
		wt.translation = wt.translation + delta;
		wt.Update();
	}

	// 左右キーでY軸回転（ラジアン単位）
	const float rotSpeedDeg = 180.0f; // degree/s
	const float rotSpeed = rotSpeedDeg * std::numbers::pi_v<float> / 180.0f;
	float yawDelta = 0.0f;
	if(CalyxFoundation::Input::PushKey(DIK_LEFT)) yawDelta -= rotSpeed * dt;
	if(CalyxFoundation::Input::PushKey(DIK_RIGHT)) yawDelta += rotSpeed * dt;

	if(std::abs(yawDelta) > 0.0f) {
		auto& wt = GetWorldTransform();
		// eulerRotation は {pitch, yaw, roll} の順で保持されている想定
		wt.eulerRotation.y += yawDelta;
		wt.rotationSource = RotationSource::Euler;
		wt.Update();
	}

	// 基底更新（アニメやコンポーネント処理）
	Actor::Update(dt);
}
