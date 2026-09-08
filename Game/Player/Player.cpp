#include "Player.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Input/Input.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>

// game
#include <Game/Audio/GameAudio.h>
#include <Game/Floater/BodyNode.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Meteorite/InGame/MeteoriteForecast.h>
#include <Game/UI/UiSprite.h>

// std
#include <algorithm>
#include <cmath>
#include <numbers>
#include <nlohmann/json.hpp>

namespace {
	constexpr const char* kKeyboardTexture =
		"Textures/GameUI/operationKey.png";
	constexpr const char* kPadTexture =
		"Textures/GameUI/operationPad.png";

	// 入力デバイスの判定用。スティックとトリガーは遊びを越えたときだけ触ったと見る
	constexpr float kDeviceStickThresholdSq = 0.3f * 0.3f;
	constexpr float kDeviceTriggerThreshold = 0.2f;

	constexpr CalyxFoundation::PadButton kAllPadButtons[] = {
		CalyxFoundation::PadButton::A,
		CalyxFoundation::PadButton::B,
		CalyxFoundation::PadButton::X,
		CalyxFoundation::PadButton::Y,
		CalyxFoundation::PadButton::LB,
		CalyxFoundation::PadButton::RB,
		CalyxFoundation::PadButton::BACK,
		CalyxFoundation::PadButton::START,
		CalyxFoundation::PadButton::L_STICK,
		CalyxFoundation::PadButton::R_STICK,
		CalyxFoundation::PadButton::DPAD_UP,
		CalyxFoundation::PadButton::DPAD_DOWN,
		CalyxFoundation::PadButton::DPAD_LEFT,
		CalyxFoundation::PadButton::DPAD_RIGHT,
	};

	/// このフレームにパッドを触っているか
	bool IsGamepadTouched() {

		for (const CalyxFoundation::PadButton button : kAllPadButtons) {
			if (CalyxFoundation::Input::PushGamepadButton(button)) {
				return true;
			}
		}

		// スティックはデッドゾーン処理済みの値が返る
		if (CalyxFoundation::Input::GetLeftStick().LengthSquared() > kDeviceStickThresholdSq ||
			CalyxFoundation::Input::GetRightStick().LengthSquared() > kDeviceStickThresholdSq) {
			return true;
		}

		// 回転入力がトリガーなので、ここを見ないと回している間だけキーボード表示に戻る
		return CalyxFoundation::Input::GetLeftTrigger() > kDeviceTriggerThreshold ||
			CalyxFoundation::Input::GetRightTrigger() > kDeviceTriggerThreshold;
	}

	/// このフレームにキーボードを触っているか。
	bool IsKeyboardTouched() {

		for (uint32_t key = 0; key < 256; key++) {
			if (CalyxFoundation::Input::PushKey(key)) {
				return true;
			}
		}
		return false;
	}
}

// ExportChain()をクリアシーンに移る際に呼ぶ
// ResultCarry::Clear()は次ステージのシーンリクエスト前に呼び出し(保存した連結のリセット)
// ResultCarry::stageIndexには次ステージに移行前(ResultCarry::Clear()後)にステージのIndexを渡す

Player::Player()
	: Actor("plane.obj", "Player") {
	// パラメータをロード（パラメータデータベースから既定値を読み込む）
	param_.LoadParams();

}

Player::~Player() {
	GameAudio::StopSe();

	// 予報を出したままシーンが切り替わっても、次のシーンが止まったままにならないように
	if (forecastPaused_) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		forecastPaused_ = false;
	}
}

void Player::DerivativeGui() {
	using namespace GuiCmd;
	if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		// SerializableObject ベースの param_ を GUI 表示
		GuiCmd::SceneObjectReferenceField("Target(FloaterManager)", floaterManager_);
		ImGui::DragFloat("stageScale", &stageScale_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("stageClearLength", &stageClearLength_, 0.01f, 1.0f, 50.0f);
		PropertyText("Manager", "%s", floaterManager_.Resolve() ? "OK" : "MISSING");
		PropertyText("Connected", "%d", static_cast<int>(chain_.size()) - 1);
		PropertyText("Anchors", "%d", static_cast<int>(handAnchors_.size()));
		PropertyText("NearestHand", "%.2f m", debugNearestDist_);
		PropertyText("AngleRejects", "%d", debugAngleRejects_);
		const int connected = static_cast<int>(chain_.size()) - 1;
		if (connected > 0) {
			debugBreakIndex_ = std::clamp(debugBreakIndex_, 1, connected);
			ImGui::DragInt("Break Index", &debugBreakIndex_, 1.0f, 1, connected);
			if (ImGui::Button("Break")) {
				chain_[debugBreakIndex_].floater->MarkBreak();
			}
		}
		ImGui::Checkbox("Result Mode", &resultMode_);
		param_.ShowGui();
		EndSection();
	}
}

void Player::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void Player::Initialize() {
	Actor::Initialize();
	DisableGravity();

	if (resultMode_) {
		stageScale_ = ResultCarry::stageScale;
	} else {
		InitializeControlUi();
	}
	auto& wt = GetWorldTransform();
	wt.scale = CalyxEngine::Vector3::One() * stageScale_;

	if (Collider* collider = GetCollider()) {
		ColliderConfig config = collider->ExtractConfig();
		config.size = { stageScale_, stageScale_, stageScale_ };
		collider->ApplyConfig(config);
	}

	chain_.clear();
	Member self{};
	for (int i = 0; i < BodyNode::kHandCount; i++) {
		self.handLocal[i] = BodyNode::Hand(i, stageScale_);
	}
	chain_.push_back(self);

	prevSelfPos_ = GetWorldTransform().translation;
	prevSelfYaw_ = GetWorldTransform().eulerRotation.y;

	HandConnectEffect_.Load("HandConnectParticle");

	if (auto manager = floaterManager_.Resolve()) {
		manager->SetStageScale(stageScale_);
		// 湧きは manager の初回 Update なので、ここで渡せば間に合う
		manager->SetFieldLimits(param_.fieldMinX, param_.fieldZLimit);
	}

	ResultCarry::stageScale = stageScale_;
	ResultCarry::stageClearDirection = stageClearLength_;

	isResultChain_ = false;
}

namespace {
	// ワールド座標のXZ平面での入力マッピング。
	// move.x = AD (X軸), move.y = WS (Z軸)
	CalyxEngine::Vector3 BuildWorldMoveDirection(const CalyxEngine::Vector2& move) {
		CalyxEngine::Vector3 direction{ move.x, 0.0f, move.y };

		// 斜め入力で速くならないよう正規化
		if (direction.LengthSquared() > 1.0f) {
			direction = direction.Normalize();
		}

		return direction;
	}

	constexpr float kMoveSeYawThreshold = 0.5f; // 移動SEを鳴らす角速度の下限
}

void Player::Update(float dt) {
	if (resultMode_) {
		if (!restored_) { restored_ = BeginRestoreChain(); } else { RestoreChainStep(dt); }
		if (auto manager = floaterManager_.Resolve()) {
			BreakChain(*manager);
		}
		ApplyChainTransforms();
		Actor::Update(dt);
		return;
	}

	// 天気予報はここで生やす。
	// Initialize では resultMode_ がまだシーンから入っておらず、
	// リザルトでも作ってしまう。上の早期 return を抜けた時点なら確実にゲーム中。
	SpawnForecast();
	UpdateForecastPause();

	// 予報を出す前に判定しておくと、生えるまでの1フレームだけ操作説明が映る。
	// デバイスの判定は予報中も回し続け、表示だけ ApplyControlUiLayout で引っ込める
	UpdateControlDevice();
	ApplyControlUiLayout();

	// 予報を見せている間は自機も止める。閉じる入力は予報側が自分で見ている
	if (IsForecastWaiting()) {
		Actor::Update(dt);
		return;
	}

	// 入力更新
	input_.Update();
	const PlayerInputState& state = input_.GetState();

	prevSelfPos_ = GetWorldTransform().translation;
	prevSelfYaw_ = GetWorldTransform().eulerRotation.y;

	// 移動
	CalyxEngine::Vector3 worldDir = BuildWorldMoveDirection(state.move);
	const float moveSpeed = param_.moveSpeed * (1.0f + (stageScale_ * 1.0f) * param_.moveSpeedScaleGain); // m/s
	if (worldDir.LengthSquared() > 0.0f) {
		// 移動量を加算（物理は使わないシンプル実装）
		CalyxEngine::Vector3 delta = worldDir * (moveSpeed * dt);
		auto& wt = GetWorldTransform();
		wt.translation = wt.translation + delta;
		wt.Update();
	}

	ClampToField();

	// 繋いだ人数ぶん遅くなる
	const float rotSpeed = CurrentTurnSpeed();

	// ユーザー入力から目標角速度を決定
	// 左右キーは残すが、主要な回転入力はゲームパッドのトリガー（LT/RT）で受け付ける
	float targetAngularVel = 0.0f;
	if (CalyxFoundation::Input::PushKey(DIK_LEFT)) targetAngularVel -= rotSpeed;
	if (CalyxFoundation::Input::PushKey(DIK_RIGHT)) targetAngularVel += rotSpeed;

	// ゲームパッドのトリガー（0.0 - 1.0）を回転入力として扱う
	const float leftTrigger = CalyxFoundation::Input::GetLeftTrigger();
	const float rightTrigger = CalyxFoundation::Input::GetRightTrigger();
	// 両トリガーの差で回転方向を決定（右トリガーが押されているほど正回転）
	const float triggerInput = rightTrigger - leftTrigger; // -1..1
	const float triggerDeadzone = 0.05f;
	if (std::abs(triggerInput) > triggerDeadzone) {
		// デッドゾーン除去およびスケール適用
		float scaled = (std::abs(triggerInput) - triggerDeadzone) / (1.0f - triggerDeadzone);
		scaled = std::copysign(scaled, triggerInput);
		targetAngularVel += scaled * rotSpeed;
	}

	// yaw の慣性（線形補間的に角速度を変化させる）
	// PlayerParam 内の yawAcceleration を使用
	yawVelocity_ += (targetAngularVel - yawVelocity_) * std::clamp(param_.yawAcceleration * dt, 0.0f, 1.0f);

	if (std::abs(yawVelocity_) > 1e-6f) {
		auto& wt = GetWorldTransform();
		// eulerRotation は {pitch, yaw, roll} の順で保持されている想定
		wt.eulerRotation.x = std::numbers::pi_v<float> / 2.0f;
		wt.eulerRotation.y += yawVelocity_ * dt;
		wt.rotationSource = RotationSource::Euler;
		wt.Update();
	}

	if (worldDir.LengthSquared() > 0.0f || std::abs(yawVelocity_) > kMoveSeYawThreshold) {
		GameAudio::PlaySeLoop(GameAudio::kSeMove);
	} else {
		GameAudio::StopSe();
	}

	// 手が触れていれば繋ぐ
	if (std::shared_ptr<FloaterManager> manager = floaterManager_.Resolve()) {
		BreakChain(*manager);
		BuildHandAnchors();
		CheckConnect(*manager, dt);
	}

	// 塊は自機の姿勢から毎フレーム組み直す
	ApplyChainTransforms();

	// 基底更新（アニメやコンポーネント処理）
	Actor::Update(dt);
}

void Player::ApplyChainTransforms() {

	const auto& selfWt = GetWorldTransform();
	const CalyxEngine::Vector3 selfPos = selfWt.translation;
	const float selfYaw = selfWt.eulerRotation.y;

	// 先頭は自機自身なので飛ばす
	for (size_t i = 1; i < chain_.size(); i++) {
		const Member& member = chain_[i];
		if (!member.floater) {
			continue;
		}

		member.floater->SetChainedTransform(
			selfPos + BodyNode::RotateY(member.offset, selfYaw),
			selfYaw + member.localAngle);
	}
}

float Player::CurrentTurnSpeed() const {

	const float base = param_.rotSpeedDeg * std::numbers::pi_v<float> / 180.0f;
	if (chain_.empty()) {
		return base;
	}
	const float count = static_cast<float>(chain_.size() - 1);
	return base / (1.0f + count * param_.heaviness);
}

bool Player::BreakChain(FloaterManager& manager) {

	breakMarks_.assign(
		chain_.size(),
		false
	);

	bool noneBroken = true;

	for (size_t i = 1; i < chain_.size(); ++i) {

		const Member& member = chain_[i];

		if (member.parent < 0 ||
			static_cast<size_t>(member.parent)
			>= chain_.size()) {

			continue;
		}

		const bool selfHit =
			member.floater &&
			member.floater->IsBreakMarked();

		if (selfHit ||
			breakMarks_[member.parent]) {

			breakMarks_[i] = true;
			noneBroken = false;
		}
	}

	if (noneBroken) {
		return false;
	}

	int writeIndex = 0;

	remap_.assign(
		chain_.size(),
		-1
	);

	for (size_t i = 0;
		i < chain_.size();
		++i) {

		if (breakMarks_[i]) {

			manager.Reclaim(
				std::move(
					chain_[i].floater
				)
			);

			continue;
		}

		remap_[i] = writeIndex;

		if (i != 0) {

			const int oldParent =
				chain_[i].parent;

			if (oldParent < 0 ||
				static_cast<size_t>(oldParent)
				>= remap_.size()) {

				continue;
			}

			chain_[i].parent =
				remap_[oldParent];
		}

		if (i != static_cast<size_t>(
			writeIndex)) {

			chain_[writeIndex] =
				std::move(chain_[i]);
		}

		++writeIndex;
	}

	chain_.erase(
		chain_.begin() + writeIndex,
		chain_.end()
	);

	return true;
}

bool Player::RestoreChain() {
	auto manager = floaterManager_.Resolve();
	if (!manager) return false;

	auto& wt = GetWorldTransform();
	wt.rotationSource = RotationSource::Euler;
	wt.Update();

	chain_.clear();
	for (size_t i = 0; i < ResultCarry::chain.size(); i++) {
		const ChainMemberData& d = ResultCarry::chain[i];
		std::shared_ptr<Floater> floater;
		if (i != 0) {                                  // [0] は自機
			floater = manager->CreateChained(d.offset);
			if (floater) {
				floater->RestoreChained();
				floater->SetStageScale(stageScale_);
				floater->ApplyStageScale();
			}
		}
		chain_.push_back(Member{ d, floater });
	}
	ApplyChainTransforms();
	return true;
}

bool Player::BeginRestoreChain() {

	// ResultScene を直接再生したときは運んできた塊が無い。
	// 復元するものが無いだけなので、Initialize が入れた自機ぶんをそのまま使って先へ進める
	if (ResultCarry::chain.empty()) {
		isResultChain_ = true;
		return true;
	}

	auto& wt = GetWorldTransform();
	wt.rotationSource = RotationSource::Euler;
	wt.Update();
	chain_.clear();
	chain_.push_back(Member{ ResultCarry::chain[0], nullptr });
	//ApplyChainTransforms();
	return true;
}

void Player::RestoreChainStep(float dt) {

	// 全員出し終わっている
	if (resultRestoreIndex_ >= ResultCarry::chain.size()) {
		isResultChain_ = true;
		return;
	}

	resultRestoreTimer_ += dt;

	if (resultRestoreTimer_ < resultRestoreInterval_) {
		return;
	}

	resultRestoreTimer_ = 0.0f;

	auto manager = floaterManager_.Resolve();
	if (!manager) {
		return;
	}

	const ChainMemberData& data =
		ResultCarry::chain[resultRestoreIndex_];

	// Floater生成
	std::shared_ptr<Floater> floater =
		manager->CreateChained(data.offset);

	if (!floater) {
		return;
	}

	// 接続済み状態にする
	floater->RestoreChained();
	floater->SetStageScale(stageScale_);
	floater->ApplyStageScale();

	GameAudio::PlaySe(GameAudio::kSeResultCount,0.6f);

	Member member{};
	static_cast<ChainMemberData&>(member) = data;
	member.floater = floater;

	chain_.push_back(std::move(member));

	// =========================================
	// 追加したFloaterのTransformを確定
	// =========================================
	ApplyChainTransforms();

	// =========================================
	// 手を繋いだ位置にエフェクトを出す
	// =========================================

	if (data.parent >= 0 &&
		static_cast<size_t>(data.parent) < chain_.size() &&
		data.parentHand >= 0 &&
		data.parentHand < BodyNode::kHandCount) {

		const auto& playerWt =
			GetWorldTransform();

		const Member& parent =
			chain_[data.parent];

		// ゲーム中のHandAnchor::posと同じ計算
		const CalyxEngine::Vector3 connectPos =
			playerWt.translation +
			BodyNode::RotateY(
				parent.handLocal[data.parentHand],
				playerWt.eulerRotation.y
			);

		HandConnectHandle_ =
			EffectAPI::Play(
				HandConnectEffect_,
				connectPos
			);
	}

	// 次のFloaterへ
	resultRestoreIndex_++;
}

void Player::ClampToField() {
	std::shared_ptr<FloaterManager> manager = floaterManager_.Resolve();
	if (!manager) {
		return;
	}

	const CalyxEngine::Vector3 center = { 0.0f,0.0f,0.0f };
	const float limit = manager->GetFieldHalfSize() - param_.fieldMargin;
	auto& wt = GetWorldTransform();

	if (wt.translation.x <= param_.fieldMinX) {
		wt.translation.x = param_.fieldMinX;
	}
	wt.translation.z = std::clamp(wt.translation.z, -param_.fieldZLimit, param_.fieldZLimit);
	if (limit <= 0.0f) {
		return;
	}

	const float x = wt.translation.x - center.x;
	const float z = wt.translation.z - center.z;
	const float distSq = x * x + z * z;

	if (distSq <= limit * limit) {
		return;
	}

	const float dist = std::sqrtf(distSq);
	wt.translation.x = center.x + x / dist * limit;
	wt.translation.z = center.z + z / dist * limit;
	wt.Update();
}

void Player::BuildHandAnchors() {

	if (chain_.empty()) {
		Member self{};
		for (int i = 0; i < BodyNode::kHandCount; i++) {
			self.handLocal[i] = BodyNode::Hand(i, stageScale_);
		}
		chain_.push_back(self);
	}

	const auto& wt = GetWorldTransform();
	const CalyxEngine::Vector3 selfPos = wt.translation;
	const float selfYaw = wt.eulerRotation.y;

	handAnchors_.clear();

	// 繋いだ手にも何人でもぶら下がれる。全メンバーの両手を候補にする
	for (size_t i = 0; i < chain_.size(); i++) {
		const Member& member = chain_[i];
		const CalyxEngine::Vector3 center = selfPos + BodyNode::RotateY(member.offset, selfYaw);

		for (int hand = 0; hand < BodyNode::kHandCount; hand++) {
			HandAnchor anchor{};
			anchor.pos = selfPos + BodyNode::RotateY(member.handLocal[hand], selfYaw);
			anchor.prevPos = prevSelfPos_ + BodyNode::RotateY(member.handLocal[hand], prevSelfYaw_);
			anchor.arm = anchor.pos - center;
			anchor.member = static_cast<int>(i);
			anchor.hand = hand;
			handAnchors_.push_back(anchor);
		}
	}
}

void Player::CheckConnect(FloaterManager& manager, float dt) {

	// 判定のしきい値。距離は2乗のまま、角度は内積のまま比べる
	const float grabRadius = param_.grabRadius * stageScale_;
	const float reachRange = param_.reachRange * stageScale_;
	const float grabSq = grabRadius * grabRadius;
	const float separationCos = std::cosf(CalyxEngine::ToRadians(param_.armSeparation));
	const float reachSq = reachRange * reachRange;

	debugAngleRejects_ = 0;
	float nearestAllSq = 1e18f;

	for (int i = static_cast<int>(manager.GetFloaters().size()) - 1; i >= 0; i--) {

		const std::shared_ptr<Floater> floater = manager.GetFloaters()[i];
		if (!floater || !floater->CanConnect()) {
			continue;
		}

		ReachToNearestHand(*floater, reachSq, dt);

		int hitIndex = -1;
		int hitHand = 0;
		for (size_t h = 0; h < handAnchors_.size() && hitIndex < 0; h++) {
			for (int own = 0; own < BodyNode::kHandCount; own++) {

				const CalyxEngine::Vector3 handPos = floater->GetHandWorld(own);
				const float distSq =
					BodyNode::DistanceSqToSegment(handPos, handAnchors_[h].prevPos, handAnchors_[h].pos);
				if (distSq < nearestAllSq) {
					nearestAllSq = distSq;
				}

				if (distSq >= grabSq) {
					continue;
				}

				const CalyxEngine::Vector3 arm = floater->GetArmWorld(own);
				if (CalyxEngine::Vector3::Dot(arm.Normalize(), handAnchors_[h].arm.Normalize()) > separationCos) {
					debugAngleRejects_++;
					continue;
				}

				hitIndex = static_cast<int>(h);
				hitHand = own;
				break;
			}
		}

		if (hitIndex < 0) {
			continue;
		}

		const HandAnchor anchor = handAnchors_[hitIndex];

		std::shared_ptr<Floater> detached = manager.Detach(floater.get());
		if (!detached) {
			continue;
		}
		Attach(detached, anchor, hitHand);

		const Member& added = chain_.back();
		const auto& wt = GetWorldTransform();
		const CalyxEngine::Vector3 center =
			wt.translation + BodyNode::RotateY(added.offset, wt.eulerRotation.y);

		for (int hand = 0; hand < BodyNode::kHandCount; hand++) {
			HandAnchor extra{};
			extra.pos = wt.translation + BodyNode::RotateY(added.handLocal[hand], wt.eulerRotation.y);
			extra.prevPos = extra.pos;
			extra.arm = extra.pos - center;
			extra.member = static_cast<int>(chain_.size()) - 1;
			extra.hand = hand;
			handAnchors_.push_back(extra);
		}
	}

	debugNearestDist_ = (nearestAllSq < 1e17f) ? std::sqrtf(nearestAllSq) : -1.0f;
}

void Player::ReachToNearestHand(Floater& floater, float reachSq, float dt) {

	if (param_.reachSpeed <= 0.0f) {
		return;
	}

	// 手の届く範囲で一番近い組を選ぶ
	int nearest = -1;
	int nearestHand = 0;
	float nearestSq = reachSq;

	for (size_t h = 0; h < handAnchors_.size(); h++) {
		for (int own = 0; own < BodyNode::kHandCount; own++) {
			const float distSq = BodyNode::DistanceSqToSegment(
				floater.GetHandWorld(own), handAnchors_[h].prevPos, handAnchors_[h].pos);
			if (distSq < nearestSq) {
				nearestSq = distSq;
				nearest = static_cast<int>(h);
				nearestHand = own;
			}
		}
	}

	if (nearest < 0) {
		return;
	}

	const HandAnchor& anchor = handAnchors_[nearest];
	const float selfYaw = GetWorldTransform().eulerRotation.y;

	usedAngles_.clear();
	usedAngles_.push_back(std::atan2f(anchor.arm.x, anchor.arm.z));

	for (const Member& member : chain_) {
		if (member.parent != anchor.member || member.parentHand != anchor.hand) {
			continue;
		}
		const CalyxEngine::Vector3 arm =
			BodyNode::RotateY(BodyNode::kHand[member.joinHand], selfYaw + member.localAngle);
		usedAngles_.push_back(std::atan2f(arm.x, arm.z));
	}

	std::sort(usedAngles_.begin(), usedAngles_.end());
	float bestGap = 0.0f;
	float bestMid = usedAngles_[0] + CalyxEngine::kPi;
	for (size_t i = 0; i < usedAngles_.size(); i++) {
		const float from = usedAngles_[i];
		const float to = (i + 1 < usedAngles_.size()) ? usedAngles_[i + 1] : usedAngles_[0] + CalyxEngine::kTwoPi;
		if (to - from > bestGap) {
			bestGap = to - from;
			bestMid = from + (to - from) * 0.5f;
		}
	}

	const float targetArmAngle =
		bestMid + floater.GetReachBias() * CalyxEngine::ToRadians(param_.reachSpread);

	floater.ReachTowardArmAngle(nearestHand, targetArmAngle, param_.reachSpeed * dt);
}

void Player::Attach(const std::shared_ptr<Floater>& floater, const HandAnchor& anchor, int ownHand) {

	const auto& selfWt = GetWorldTransform();
	const float selfYaw = selfWt.eulerRotation.y;

	const Member& target = chain_[anchor.member];

	Member member{};
	member.floater = floater;

	// 向きは漂っていたときのまま
	const float drift = floater->GetYaw() - selfYaw;
	member.localAngle = drift + BodyNode::WrapAngle(target.localAngle - drift) * param_.alignToChain;

	member.offset =
		target.handLocal[anchor.hand] - BodyNode::RotateY(BodyNode::Hand(ownHand, stageScale_), member.localAngle);
	member.offset.y = floater->GetWorldTransform().GetWorldPosition().y - selfWt.translation.y;

	for (int i = 0; i < BodyNode::kHandCount; i++) {
		member.handLocal[i] = member.offset + BodyNode::RotateY(BodyNode::Hand(i, stageScale_), member.localAngle);
	}

	member.parent = anchor.member;
	member.parentHand = anchor.hand;
	member.joinHand = ownHand;

	floater->MarkChained(ownHand);

	// 親子付けは使わず、この場でワールド姿勢を確定させる。
	// 以降は ApplyChainTransforms が毎フレーム同じ式で組み直す
	floater->SetChainedTransform(
		selfWt.translation + BodyNode::RotateY(member.offset, selfYaw),
		selfYaw + member.localAngle);

	chain_.push_back(member);

	HandConnectHandle_ = EffectAPI::Play(HandConnectEffect_, anchor.pos);
}

void Player::ApplyConfigFromJson(const nlohmann::json& j) {
	// まず基底の設定を適用して Transform / Collider 等を復元する
	// Actor が基底クラスとして JSON の基本項目を処理する想定
	Actor::ApplyConfigFromJson(j);

	// シーン保存時はオブジェクト固有のキーでネストされる場合があるため対応する。
	const std::string typeKey(GetTypeName());
	const nlohmann::json* src = &j;
	if (j.contains(typeKey)) {
		src = &j.at(typeKey);
	}

	param_.moveSpeed = src->value("moveSpeed", param_.moveSpeed);
	param_.rotSpeedDeg = src->value("rotSpeedDeg", param_.rotSpeedDeg);
	param_.yawAcceleration = src->value("yawAcceleration", param_.yawAcceleration);
	resultMode_ = src->value("resultMode", resultMode_);
	floaterManager_ = src->value("FloaterManagerPtr", floaterManager_);
	stageScale_ = src->value("stageScale", stageScale_);
	stageClearLength_ = src->value("stageClearLength", stageClearLength_);
}

void Player::ExtractConfigToJson(nlohmann::json& j) const {
	// まず基底の項目を JSON に書き出す（Transform / Collider 等）
	Actor::ExtractConfigToJson(j);

	const std::string typeKey(GetTypeName());
	nlohmann::json derived;
	derived["moveSpeed"] = param_.moveSpeed;
	derived["rotSpeedDeg"] = param_.rotSpeedDeg;
	derived["yawAcceleration"] = param_.yawAcceleration;
	derived["resultMode"] = resultMode_;
	derived["FloaterManagerPtr"] = floaterManager_;
	derived["stageScale"] = stageScale_;
	derived["stageClearLength"] = stageClearLength_;
	if (!derived.empty()) {
		j[typeKey] = std::move(derived);
	}
}

void Player::AllBreak() {

	// 0番目はPlayer自身なので飛ばす
	for (size_t i = 1; i < chain_.size(); i++) {

		if (chain_[i].floater) {
			chain_[i].floater->MarkBreak();
		}
	}
}

void Player::InitializeControlUi() {

	// プレハブ用の一時コピーからSpriteを撒かない
	if (IsTransient() || controlUIKeyboard_) {
		return;
	}

	controlUIKeyboard_ = SceneAPI::Instantiate<UiSprite>(kKeyboardTexture);
	controlUIPad_ = SceneAPI::Instantiate<UiSprite>(kPadTexture);

	// 中心基準にしておくと、画面の右下から余白ぶん戻すだけで置ける
	for (const std::shared_ptr<UiSprite>& sprite :
		{ controlUIKeyboard_, controlUIPad_ }) {
		if (!sprite) {
			continue;
		}
		sprite->SetAnchor({ 0.5f, 0.5f });
		sprite->SetRotationDeg(0.0f);
		sprite->SetColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// パッドに触るまではキーボードを出しておく
	controlDevice_ = ControlDevice::Keyboard;
	ApplyControlUiLayout();
}

void Player::UpdateControlDevice() {

	const bool gamepadTouched = IsGamepadTouched();
	const bool keyboardTouched = IsKeyboardTouched();

	// 両方触っている / どちらも触っていないフレームは、今出している側を保つ
	if (gamepadTouched == keyboardTouched) {
		return;
	}

	controlDevice_ =
		gamepadTouched ? ControlDevice::Gamepad : ControlDevice::Keyboard;
}

void Player::ApplyControlUiLayout() {

	// 予報の板が映っている間は操作説明を引っ込める。
	// スタート時の待ちも覗き見も IsVisible() でまとめて拾える
	const bool hideAll = forecast_ && forecast_->IsVisible();

	// 2枚を同じ場所に重ねて置き、表示だけ入れ替える
	const bool useGamepad = controlDevice_ == ControlDevice::Gamepad;
	ApplyControlUiSprite(controlUIKeyboard_, !hideAll && !useGamepad);
	ApplyControlUiSprite(controlUIPad_, !hideAll && useGamepad);
}

void Player::ApplyControlUiSprite(
	const std::shared_ptr<UiSprite>& sprite, bool visible) {

	if (!sprite) {
		return;
	}

	sprite->SetSizePx(param_.controlUiWidth, param_.controlUiHeight);
	sprite->SetPositionPx(param_.controlUiCenterX, param_.controlUiCenterY);
	sprite->SetOrderInLayer(param_.controlUiOrderInLayer);
	sprite->SetVisible(visible);
}

void Player::SpawnForecast() {

	if (forecast_ || IsTransient()) {
		return;
	}

	if (ResultCarry::stageIndex <= 2) {
		return;
	}

	forecast_ = SceneAPI::Instantiate<MeteoriteForecast>();
	if (!forecast_) {
		return;
	}

	// Instantiate は Initialize を呼ばない
	forecast_->Initialize();
	forecast_->SetParent(shared_from_this(), false);
	forecast_->ShowAtStart();
}

void Player::UpdateForecastPause() {

	const bool waiting = IsForecastWaiting();
	if (waiting == forecastPaused_) {
		return;
	}

	// Floater も Meteorite も Spawner も dt だけで動くので、
	// ここを 0 にすれば個別に手を入れなくても全部止まる
	ClockManager::GetInstance()->SetTimeScale(waiting ? 0.0f : 1.0f);
	forecastPaused_ = waiting;
}

bool Player::IsForecastWaiting() const {
	return forecast_ && forecast_->IsWaitingAtStart();
}

bool Player::IsConnectedFloaterHandInsideRadius(
	const CalyxEngine::Vector3& center,
	float radius) const {

	if (radius <= 0.0f) {
		return false;
	}

	const float radiusSq = radius * radius;

	// [0] はPlayer自身なので、接続中Floaterだけを見る
	for (size_t i = 1; i < chain_.size(); ++i) {
		const Member& member = chain_[i];
		if (!member.floater) {
			continue;
		}

		for (int hand = 0; hand < BodyNode::kHandCount; ++hand) {
			const CalyxEngine::Vector3 handPos =
				member.floater->GetHandWorld(hand);

			const CalyxEngine::Vector3 diff = handPos - center;
			if (diff.LengthSquared() <= radiusSq) {
				return true;
			}
		}
	}

	return false;
}

bool Player::GetConnectedFloaterHandXRange(
	float& minX,
	float& maxX) const {

	bool foundHand = false;

	minX = 0.0f;
	maxX = 0.0f;

	// [0]はPlayer自身なので除外
	for (size_t i = 1; i < chain_.size(); ++i) {

		const Member& member = chain_[i];

		if (!member.floater) {
			continue;
		}

		// Floaterの両手を見る
		for (int hand = 0;
			hand < BodyNode::kHandCount;
			++hand) {

			const CalyxEngine::Vector3 handPos =
				member.floater->GetHandWorld(hand);

			if (!foundHand) {

				// 最初に見つかった手を初期値にする
				minX = handPos.x;
				maxX = handPos.x;

				foundHand = true;
				continue;
			}

			minX = std::min(minX, handPos.x);
			maxX = std::fmax(maxX, handPos.x);
		}
	}

	return foundHand;
}

float Player::GetConnectedFloaterHandXDistance() const {

	float minX = 0.0f;
	float maxX = 0.0f;

	if (!GetConnectedFloaterHandXRange(
		minX,
		maxX)) {

		return 0.0f;
	}

	return maxX - minX;
}

void Player::ExportChain() const {
	ResultCarry::chain.assign(chain_.begin(), chain_.end());
}

void Player::SetupResult() {
	resultMode_ = true;
}