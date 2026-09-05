#include "Player.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Input/Input.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/Audio/GameAudio.h>
#include <Game/Floater/BodyNode.h>
#include <Game/Floater/Floater.h>
#include <Game/Result/ResultCarry.h>
#include <Game/Meteorite/InGame/MeteoriteForecast.h>

// std
#include <algorithm>
#include <cmath>
#include <numbers>
#include <nlohmann/json.hpp>

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
}

void Player::DerivativeGui() {
	using namespace GuiCmd;
	if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		// SerializableObject ベースの param_ を GUI 表示
		GuiCmd::SceneObjectReferenceField("Target(FloaterManager)", floaterManager_);
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

	chain_.clear();
	Member self{};
	for (int i = 0; i < BodyNode::kHandCount; i++) {
		self.handLocal[i] = BodyNode::kHand[i];
	}
	chain_.push_back(self);

	prevSelfPos_ = GetWorldTransform().translation;
	prevSelfYaw_ = GetWorldTransform().eulerRotation.y;

	HandConnectEffect_.Load("HandConnectParticle");
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
		if (!restored_) { restored_ = RestoreChain(); }
		else { RestoreChainStep(dt); }
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

	// 入力更新
	input_.Update();
	const PlayerInputState& state = input_.GetState();

	prevSelfPos_ = GetWorldTransform().translation;
	prevSelfYaw_ = GetWorldTransform().eulerRotation.y;

	// 移動
	CalyxEngine::Vector3 worldDir = BuildWorldMoveDirection(state.move);
	const float moveSpeed = param_.moveSpeed; // m/s
	if (worldDir.LengthSquared() > 0.0f) {
		// 移動量を加算（物理は使わないシンプル実装）
		CalyxEngine::Vector3 delta = worldDir * (moveSpeed * dt);
		auto& wt = GetWorldTransform();
		wt.translation = wt.translation + delta;
		wt.Update();
	}

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
	breakMarks_.assign(chain_.size(), false);
	bool noneBroken = true;
	for (size_t i = 1; i < chain_.size(); i++) {
		const Member& member = chain_[i];
		const bool selfHit = member.floater && member.floater->IsBreakMarked();
		if (selfHit || breakMarks_[member.parent]) {
			breakMarks_[i] = true;
			noneBroken = false;
		}
	}
	if (noneBroken) return false;

	int writeIndex = 0;
	remap_.assign(chain_.size(), -1);
	for (size_t i = 0; i < chain_.size(); i++) {
		if (breakMarks_[i]) {
			manager.Reclaim(std::move(chain_[i].floater));
			continue;
		}

		remap_[i] = writeIndex;
		if (i != 0) {
			chain_[i].parent = remap_[chain_[i].parent];
		}
		if (i != static_cast<size_t>(writeIndex)) {
			chain_[writeIndex] = std::move(chain_[i]);
		}
		writeIndex++;
	}

	chain_.erase(chain_.begin() + writeIndex, chain_.end());
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
			}
		}
		chain_.push_back(Member{ d, floater });
	}
	ApplyChainTransforms();
	return true;
}

void Player::RestoreChainStep(float dt) {
	// 全員出し終わっている
	if (resultRestoreIndex_ >= ResultCarry::chain.size()) {
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

	Member member{};
	static_cast<ChainMemberData&>(member) = data;
	member.floater = floater;

	chain_.push_back(std::move(member));

	// 次のFloaterへ
	resultRestoreIndex_++;
}

void Player::BuildHandAnchors() {

	if (chain_.empty()) {
		Member self{};
		for (int i = 0; i < BodyNode::kHandCount; i++) {
			self.handLocal[i] = BodyNode::kHand[i];
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
	const float grabSq = param_.grabRadius * param_.grabRadius;
	const float separationCos = std::cosf(CalyxEngine::ToRadians(param_.armSeparation));
	const float reachSq = param_.reachRange * param_.reachRange;

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
		target.handLocal[anchor.hand] - BodyNode::RotateY(BodyNode::kHand[ownHand], member.localAngle);
	member.offset.y = floater->GetWorldTransform().GetWorldPosition().y - selfWt.translation.y;

	for (int i = 0; i < BodyNode::kHandCount; i++) {
		member.handLocal[i] = member.offset + BodyNode::RotateY(BodyNode::kHand[i], member.localAngle);
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

void Player::SpawnForecast() {

	if (forecast_ || IsTransient()) {
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

bool Player::IsForecastWaiting() const {
	return forecast_ && forecast_->IsWaitingAtStart();
}

void Player::ExportChain() const {
	ResultCarry::chain.assign(chain_.begin(), chain_.end());
}

void Player::SetupResult() {
	resultMode_ = true;
}