#include "MeteoriteWarning.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include "FallingMeteorite.h"

// std
#include <numbers>

namespace {
	constexpr float kPitch = std::numbers::pi_v<float> *0.5f;

	// Resources/Assets/Prefabs/ からの相対パス
	constexpr const char* kFallingMeteoritePrefab = "FallingMeteorite.prefab";
}

MeteoriteWarning::MeteoriteWarning()
	: Actor("plane.obj", "MeteoriteWarning") {

	// 置いた直後から地面に寝ているようにする。
	// コンストラクタで入れておけば、シーンに保存された回転のほうが後から勝つ
	worldTransform_.eulerRotation.x = kPitch;
	worldTransform_.rotationSource = RotationSource::Euler;
}

void MeteoriteWarning::Initialize() {
	Actor::Initialize();

	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();

	// Actor::Update を呼ぶ以上、切らないと置いた場所から落ちていく
	DisableGravity();

	SetTexture("Textures/circle/groundPrediction.png");

	// phase_ たちはメンバ初期化子で既定値が入っている。
	// ここで入れ直すと、Initialize より先に Start() が来たときに号令を潰す
}

void MeteoriteWarning::Update(float dt) {

	switch (phase_) {
	case Phase::Waiting:  UpdateWaiting(dt);  break;
	case Phase::Blinking: UpdateBlinking(dt); break;
	case Phase::Falling:  UpdateFalling(dt);  break;

	case Phase::Idle:
	case Phase::Done:
	default:
		break;
	}

	Actor::Update(dt);
}

void MeteoriteWarning::Start(const MeteoriteFallSettings& settings) {

	// 1地点につき1回だけ。2回目の号令は無視する
	if (phase_ != Phase::Idle) {
		return;
	}

	settings_ = settings;
	timer_ = 0.0f;
	blinkedCount_ = 0;
	phase_ = Phase::Waiting;
}

void MeteoriteWarning::UpdateWaiting(float dt) {

	timer_ += dt;
	if (timer_ < param_.delay) {
		return;
	}

	timer_ = 0.0f;
	blinkedCount_ = 0;

	// 点滅なしの設定ならそのまま落とす
	if (settings_.blinkTimes <= 0 || settings_.blinkPeriod <= 0.0f) {
		DropMeteorite();
		return;
	}
	phase_ = Phase::Blinking;
}

void MeteoriteWarning::UpdateBlinking(float dt) {

	timer_ += dt;

	// 1周期の前半だけ見せる
	SetDrawEnable(timer_ < settings_.blinkPeriod * 0.5f);

	if (timer_ < settings_.blinkPeriod) {
		return;
	}

	timer_ -= settings_.blinkPeriod;
	blinkedCount_++;

	if (blinkedCount_ >= settings_.blinkTimes) {
		DropMeteorite();
	}
}

void MeteoriteWarning::UpdateFalling([[maybe_unused]] float dt) {

	// 落とした隕石が着弾して判定を出し終えるまで待つ
	if (!meteorite_ || meteorite_->IsFinished()) {
		phase_ = Phase::Done;
	}
}

void MeteoriteWarning::DropMeteorite() {

	// 円を消してから落とす
	SetDrawEnable(false);
	phase_ = Phase::Falling;

	const CalyxEngine::Vector3 impactPos = GetWorldTransform().GetWorldPosition();

	std::shared_ptr<FallingMeteorite> meteorite =
		SceneAPI::InstantiatePrefabRoot<FallingMeteorite>(kFallingMeteoritePrefab, impactPos);
	if (!meteorite) {
		// プレハブが無い / 生成に失敗しても止まらないよう、そのまま完了にする
		phase_ = Phase::Done;
		return;
	}
	meteorite->Initialize();

	// 落下開始位置は Fall が impactPos の真上へ置き直す
	meteorite->Fall(impactPos,
					settings_.fallHeight,
					settings_.fallSpeed,
					settings_.colliderRadius,
					settings_.impactHold);

	meteorite_ = std::move(meteorite);
}

void MeteoriteWarning::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void MeteoriteWarning::DerivativeGui() {

	static const char* kPhaseNames[] = { "Idle", "Waiting", "Blinking", "Falling", "Done" };
	ImGui::Text("Phase  : %s", kPhaseNames[static_cast<int>(phase_)]);
	ImGui::Text("Timer  : %.2f s", timer_);
	ImGui::Text("Blinked: %d", blinkedCount_);
	param_.ShowGui();
}
