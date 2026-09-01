#include "Player.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Input/Input.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"

// std
#include <numbers>
#include <nlohmann/json.hpp>

Player::Player()
	: Actor("plane.obj", "Player") {
	// パラメータをロード（パラメータデータベースから既定値を読み込む）
	param_.LoadParams();
}

void Player::DerivativeGui() {
	using namespace GuiCmd;
	if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		// SerializableObject ベースの param_ を GUI 表示
		param_.ShowGui();
		EndSection();
	}
}

void Player::Initialize() {
	Actor::Initialize();

	auto& wt = GetWorldTransform();
	// 常に X 軸に -90度回転させるため、回転ソースをオイラーにして固定ピッチを設定する
	wt.eulerRotation.x = std::numbers::pi_v<float> * 0.5f; // pitch = -90deg
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
	const float moveSpeed = param_.moveSpeed; // m/s
	if(worldDir.LengthSquared() > 0.0f) {
		// 移動量を加算（物理は使わないシンプル実装）
		CalyxEngine::Vector3 delta = worldDir * (moveSpeed * dt);
		auto& wt = GetWorldTransform();
		wt.translation = wt.translation + delta;
		wt.Update();
	}

	// 左右キーでY軸回転（ラジアン単位）
	const float rotSpeed = param_.rotSpeedDeg * std::numbers::pi_v<float> / 180.0f;

	// ユーザー入力から目標角速度を決定
	// 左右キーは残すが、主要な回転入力はゲームパッドのトリガー（LT/RT）で受け付ける
	float targetAngularVel = 0.0f;
	if(CalyxFoundation::Input::PushKey(DIK_LEFT)) targetAngularVel -= rotSpeed;
	if(CalyxFoundation::Input::PushKey(DIK_RIGHT)) targetAngularVel += rotSpeed;

	// ゲームパッドのトリガー（0.0 - 1.0）を回転入力として扱う
	const float leftTrigger = CalyxFoundation::Input::GetLeftTrigger();
	const float rightTrigger = CalyxFoundation::Input::GetRightTrigger();
	// 両トリガーの差で回転方向を決定（右トリガーが押されているほど正回転）
	const float triggerInput = rightTrigger - leftTrigger; // -1..1
	const float triggerDeadzone = 0.05f;
	if(std::abs(triggerInput) > triggerDeadzone) {
		// デッドゾーン除去およびスケール適用
		float scaled = (std::abs(triggerInput) - triggerDeadzone) / (1.0f - triggerDeadzone);
		scaled = std::copysign(scaled, triggerInput);
		targetAngularVel += scaled * rotSpeed;
	}

	// yaw の慣性（線形補間的に角速度を変化させる）
	// PlayerParam 内の yawAcceleration を使用
	yawVelocity_ += (targetAngularVel - yawVelocity_) * std::clamp(param_.yawAcceleration * dt, 0.0f, 1.0f);

	if(std::abs(yawVelocity_) > 1e-6f) {
		auto& wt = GetWorldTransform();
		// eulerRotation は {pitch, yaw, roll} の順で保持されている想定
		wt.eulerRotation.y += yawVelocity_ * dt;
		wt.rotationSource = RotationSource::Euler;
		wt.Update();
	}

	// 基底更新（アニメやコンポーネント処理）
	Actor::Update(dt);
}

void Player::ApplyConfigFromJson(const nlohmann::json& j) {
	// まず基底の設定を適用して Transform / Collider 等を復元する
	// Actor が基底クラスとして JSON の基本項目を処理する想定
	Actor::ApplyConfigFromJson(j);

	// シーン保存時はオブジェクト固有のキーでネストされる場合があるため対応する。
	const std::string typeKey(GetTypeName());
	const nlohmann::json* src = &j;
	if(j.contains(typeKey)) {
		src = &j.at(typeKey);
	}

	param_.moveSpeed = src->value("moveSpeed", param_.moveSpeed);
	param_.rotSpeedDeg = src->value("rotSpeedDeg", param_.rotSpeedDeg);
	param_.yawAcceleration = src->value("yawAcceleration", param_.yawAcceleration);
}

void Player::ExtractConfigToJson(nlohmann::json& j) const {
	// まず基底の項目を JSON に書き出す（Transform / Collider 等）
	Actor::ExtractConfigToJson(j);

	const std::string typeKey(GetTypeName());
	nlohmann::json derived;
	derived["moveSpeed"] = param_.moveSpeed;
	derived["rotSpeedDeg"] = param_.rotSpeedDeg;
	derived["yawAcceleration"] = param_.yawAcceleration;
	if(!derived.empty()) {
		j[typeKey] = std::move(derived);
	}
}
