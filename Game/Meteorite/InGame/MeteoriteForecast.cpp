#include "MeteoriteForecast.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include <Game/Result/ResultCarry.h>
#include <Game/Audio/GameAudio.h>

// std
#include <cmath>
#include <numbers>
#include <string>

using namespace CalyxEngine;

namespace {

	// スタート時の予報を閉じる入力
	constexpr uint32_t kCloseKey = DIK_SPACE;
	constexpr CalyxFoundation::PadButton kCloseButton = CalyxFoundation::PadButton::A;

	// ゲーム中に覗く入力。押している間だけ映る
	constexpr uint32_t kPeekKey = DIK_TAB;
	constexpr CalyxFoundation::PadButton kPeekButton = CalyxFoundation::PadButton::Y;

	// 予報の絵。ステージごとに1枚ずつ用意する
	constexpr const char* kForecastTexturePrefix = "Textures/forecast/stage";
	constexpr const char* kAreaTexturePrefix = "Textures/GameUI/area";
	constexpr const char* kForecastTextureSuffix = ".png";

	constexpr float kPlaneUnit = 0.5f;

	// 横線が伸びてから縦に開く横線の時間
	constexpr float kLinePhase = 0.35f;

	float Saturate(float value) noexcept {
		const float lower = value < 0.0f ? 0.0f : value;
		return lower > 1.0f ? 1.0f : lower;
	}

	float EaseOutCubic(float t) noexcept {
		const float inv = 1.0f - t;
		return 1.0f - inv * inv * inv;
	}

	float Deg2Rad(float degree) noexcept {
		return degree * std::numbers::pi_v<float> / 180.0f;
	}

	/// 0..1 を、両端が速く中央がゆっくりになるよう歪める。
	float FlowCurve(float t, float power) noexcept {

		if (power <= 1.0f) {
			return t;
		}

		const float s = t * 2.0f - 1.0f;
		const float shaped = std::pow(std::abs(s), power);
		return 0.5f + 0.5f * (s < 0.0f ? -shaped : shaped);
	}

	/// 板をカメラの手前に置くのに要る姿勢一式
	struct CameraBasis {
		Vector3 right{};
		Vector3 up{};
		Vector3 forward{};
		Vector3 eye{};
		Quaternion rotation{};
	};

	bool TryGetCameraBasis(CameraBasis& basis) {

		Camera3d* camera = CameraManager::GetMain3d();
		if (!camera) {
			return false;
		}

		const CalyxEngine::Matrix4x4& cameraMatrix = camera->GetWorldTransform().matrix.world;

		basis.right = Vector3{ cameraMatrix.m[0][0], cameraMatrix.m[0][1], cameraMatrix.m[0][2] }.Normalize();
		basis.up = Vector3{ cameraMatrix.m[1][0], cameraMatrix.m[1][1], cameraMatrix.m[1][2] }.Normalize();
		basis.forward = Vector3{ cameraMatrix.m[2][0], cameraMatrix.m[2][1], cameraMatrix.m[2][2] }.Normalize();
		basis.eye = { cameraMatrix.m[3][0], cameraMatrix.m[3][1], cameraMatrix.m[3][2] };
		basis.rotation = Quaternion::FromMatrix(cameraMatrix);

		return true;
	}

	Quaternion RotationOffsetOf(const Vector3& degree) {
		return Quaternion::EulerToQuaternion({
			Deg2Rad(degree.x),
			Deg2Rad(degree.y),
			Deg2Rad(degree.z) });
	}
}

MeteoriteForecast::MeteoriteForecast()
	: Actor("plane.obj", "MeteoriteForecast") {}

void MeteoriteForecast::Initialize() {
	Actor::Initialize();

	param_.LoadParams();

	DisableGravity(*this);

	// ステージ番号は次ステージへ移る前に ResultCarry へ入る運用。
	stageIndex_ = ResultCarry::stageIndex;
	ApplyStageTexture();

	SetColor(param_.color);

	auto& wt = GetWorldTransform();

	wt.inheritTranslate = false;
	wt.inheritRotate = false;
	wt.inheritScale = false;

	// 出るまでは畳んでおく
	openRate_ = 0.0f;
	SetDrawEnable(false);
}

void MeteoriteForecast::Update(float dt) {

	// 開始時の予報はゲーム時間を止めた状態で見せる
	// 展開アニメは TimeScale の影響を受けない dt で回す
	const float rawDt = ClockManager::GetInstance()->GetRawDeltaTime();

	UpdatePhase(rawDt);

	if (phase_ != Phase::Hidden) {
		UpdateVisual(rawDt);
		FollowCamera();
	}

	// 惑星名は板が畳まれた後に流れる。上の if に入れると一度も動かない
	UpdateStarName(rawDt);

	Actor::Update(dt);
}

void MeteoriteForecast::SetStage(int stageIndex) {

	stageIndex_ = stageIndex < 0 ? 0 : stageIndex;
	ApplyStageTexture();
}

void MeteoriteForecast::ApplyStageTexture() {

	SetTexture(kForecastTexturePrefix + std::to_string(stageIndex_) + kForecastTextureSuffix);

	if (IsTransient()) {
		return;
	}

	if (!starName_) {
		starName_ = SceneAPI::Instantiate<Actor>("plane.obj", "StarName");

		auto& wt = starName_->GetWorldTransform();
		wt.scale = {
				param_.nameSize.x * kPlaneUnit,
				param_.nameSize.y * kPlaneUnit,
				1.0f
		};

		DisableGravity(*starName_);
		starName_->SetDrawEnable(false);
		starName_->SetBlendMode(BlendMode::ADD);
	}

	// 絵はステージが変わるたびに貼り直す
	starName_->SetTexture(kAreaTexturePrefix + std::to_string(stageIndex_ + 1) + kForecastTextureSuffix);
}

void MeteoriteForecast::ShowAtStart() {

	openRate_ = 0.0f;
	Open(Mode::Start);
}

bool MeteoriteForecast::IsWaitingAtStart() const {

	return mode_ == Mode::Start
		&& (phase_ == Phase::Opening || phase_ == Phase::Shown);
}

void MeteoriteForecast::UpdatePhase(float dt) {

	switch (phase_) {
	case Phase::Hidden:

		// ゲーム中は覗きボタンを押している間だけ
		if (IsPeekHeld()) {
			Open(Mode::Peek);
		}
		break;

	case Phase::Opening:

		openRate_ += param_.openTime > 0.0f ? dt / param_.openTime : 1.0f;
		if (openRate_ >= 1.0f) {
			openRate_ = 1.0f;
			phase_ = Phase::Shown;
		}

		// 開ききる前に離したらそのまま畳みに入る
		if (mode_ == Mode::Peek && !IsPeekHeld()) {
			Close();
		}
		break;

	case Phase::Shown:

		if (mode_ == Mode::Start) {
			if (IsClosePressed()) {
				Close();
			}
		} else if (!IsPeekHeld()) {
			Close();
		}
		break;

	case Phase::Closing:

		openRate_ -= param_.closeTime > 0.0f ? dt / param_.closeTime : 1.0f;
		if (openRate_ <= 0.0f) {
			openRate_ = 0.0f;
			phase_ = Phase::Hidden;
			SetDrawEnable(false);

			// 開始時の予報を畳み終えたときだけ流す。板と場所が重ならないよう閉じ切ってから
			if (mode_ == Mode::Start && !nameShown_) {
				nameShown_ = true;
				BeginStarNameFlow();
			}
		} else if (mode_ == Mode::Peek && IsPeekHeld()) {
			// 畳んでいる途中で押し直されたら開き直す
			phase_ = Phase::Opening;
		}
		break;

	default:
		break;
	}
}

void MeteoriteForecast::UpdateVisual(float dt) {

	animeTime_ += dt;

	// 横線が伸びてから縦に開く
	const float lineRate = EaseOutCubic(Saturate(openRate_ / kLinePhase));
	const float panelRate = EaseOutCubic(Saturate((openRate_ - kLinePhase) / (1.0f - kLinePhase)));

	auto& wt = GetWorldTransform();
	wt.scale.x = param_.size.x * kPlaneUnit * lineRate;

	// 完全に潰さないよう下限を残す
	wt.scale.y = param_.size.y * kPlaneUnit * (0.02f + 0.98f * panelRate);
	wt.scale.z = 1.0f;

	const float flicker = std::sin(animeTime_ * param_.flickerSpeed) * param_.flickerAmp;

	Vector4 color = param_.color;
	color.w = param_.color.w * Saturate(openRate_ * (1.0f - flicker));
	//SetColor(color);
}

void MeteoriteForecast::FollowCamera() {

	CameraBasis basis;
	if (!TryGetCameraBasis(basis)) {
		return;
	}

	// 揺れ幅は板の高さに対する割合で持つ。
	// メートルで持つと distance を変えたときだけ画面上の揺れ幅が変わってしまう。
	const float bob = std::sin(animeTime_ * param_.bobSpeed) * param_.bobRate * param_.size.y;

	auto& wt = GetWorldTransform();
	wt.translation =
		basis.eye
		+ basis.forward * param_.distance
		+ basis.right * param_.offset.x
		+ basis.up * (param_.offset.y + bob);

	// カメラと同じ向きにする
	wt.rotation = Quaternion::Multiply(RotationOffsetOf(param_.rotationOffsetDeg), basis.rotation);
	wt.rotationSource = RotationSource::Quaternion;
}

void MeteoriteForecast::UpdateStarName(float dt) {

	if (!nameFlowing_ || !starName_) {
		return;
	}

	CameraBasis basis;
	if (!TryGetCameraBasis(basis)) {
		return;
	}

	nameTime_ += dt;

	const float t = param_.nameFlowTime > 0.0f
		? Saturate(nameTime_ / param_.nameFlowTime)
		: 1.0f;

	// 左の外から入り、中央で粘って、右の外へ抜ける
	const float lateral =
		param_.nameTravelX * (FlowCurve(t, param_.nameFlowPower) * 2.0f - 1.0f);

	auto& wt = starName_->GetWorldTransform();
	wt.translation =
		basis.eye
		+ basis.forward * param_.nameDistance
		+ basis.right * lateral
		+ basis.up * param_.nameHeight;

	// 板と同じ換算。毎フレーム入れているのは ImGui で大きさを詰められるようにするため
	wt.scale = {
		param_.nameSize.x * kPlaneUnit,
		param_.nameSize.y * kPlaneUnit,
		1.0f };

	wt.rotation = Quaternion::Multiply(RotationOffsetOf(param_.rotationOffsetDeg), basis.rotation);
	wt.rotationSource = RotationSource::Quaternion;

	if (t >= 1.0f) {
		nameFlowing_ = false;
		starName_->SetDrawEnable(false);
	}
}

void MeteoriteForecast::BeginStarNameFlow() {

	if (!starName_) {
		return;
	}

	nameFlowing_ = true;
	nameTime_ = 0.0f;

	// 位置を入れてから出す。順番が逆だと原点に 1 フレーム映る
	UpdateStarName(0.0f);
	starName_->SetDrawEnable(true);
}

void MeteoriteForecast::Open(Mode mode) {

	mode_ = mode;
	phase_ = Phase::Opening;
	SetDrawEnable(true);
}

void MeteoriteForecast::Close() {
	phase_ = Phase::Closing;
	GameAudio::PlaySe(GameAudio::kSeDangerClose);
}

bool MeteoriteForecast::IsClosePressed() {
	return CalyxFoundation::Input::TriggerKey(kCloseKey)
		|| CalyxFoundation::Input::TriggerGamepadButton(kCloseButton);
}

bool MeteoriteForecast::IsPeekHeld() {
	return CalyxFoundation::Input::PushKey(kPeekKey)
		|| CalyxFoundation::Input::PushGamepadButton(kPeekButton);
}

void MeteoriteForecast::DisableGravity(Actor& actor) {
	auto& movement = actor.GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void MeteoriteForecast::DerivativeGui() {

	static const char* kPhaseNames[] = { "Hidden", "Opening", "Shown", "Closing" };
	ImGui::Text("Phase : %s", kPhaseNames[static_cast<int>(phase_)]);
	ImGui::Text("Mode  : %s", mode_ == Mode::Start ? "Start" : "Peek");
	ImGui::Text("Open  : %.2f", openRate_);

	// 絵を差し替え確認する用
	int stage = stageIndex_;
	if (ImGui::InputInt("Stage", &stage)) {
		SetStage(stage);
	}

	if (ImGui::Button("Show At Start")) {
		ShowAtStart();
	}

	// 惑星名の流れ方だけ確認する用
	if (ImGui::Button("Flow Star Name")) {
		BeginStarNameFlow();
	}

	param_.ShowGui();
}

// 登録
MeteoriteForecast::ForecastParam::ForecastParam() {

	AddField("distance", distance)
		.Category("Layout")
		.Tooltip("カメラから板までの距離。近づけるほど大きく映る");

	AddField("size", size)
		.Category("Layout")
		.Tooltip("板の大きさ (m)");

	AddField("offset", offset)
		.Category("Layout")
		.Tooltip("画面内のずらし。x = 右, y = 上");

	AddField("rotationOffsetDeg", rotationOffsetDeg)
		.Category("Layout")
		.Tooltip("板が裏返って見えるときの微調整。y に 180 を入れる");

	AddField("openTime", openTime)
		.Category("Animation")
		.Tooltip("展開にかける秒数");

	AddField("closeTime", closeTime)
		.Category("Animation")
		.Tooltip("畳むのにかける秒数");

	AddField("color", color)
		.Category("Look")
		.Tooltip("投影の色。加算合成なので明るいほど強く光る");

	AddField("flickerAmp", flickerAmp)
		.Category("Look")
		.Tooltip("明滅の強さ");

	AddField("flickerSpeed", flickerSpeed)
		.Category("Look")
		.Tooltip("明滅の速さ");

	AddField("bobRate", bobRate)
		.Category("Look")
		.Tooltip("上下の揺れ幅。板の高さに対する割合なので、size を変えても見た目の比率は変わらない");

	AddField("bobSpeed", bobSpeed)
		.Category("Look")
		.Tooltip("上下の揺れの速さ");

	AddField("nameDistance", nameDistance)
		.Category("StarName")
		.Tooltip("カメラから惑星名までの距離。近づけるほど大きく映る");

	AddField("nameSize", nameSize)
		.Category("StarName")
		.Tooltip("惑星名の板の大きさ (m)");

	AddField("nameHeight", nameHeight)
		.Category("StarName")
		.Tooltip("画面内の高さ。+ で上へ");

	AddField("nameTravelX", nameTravelX)
		.Category("StarName")
		.Tooltip("左右の折り返し位置。画面外まで抜けるよう、画角の半分より大きめに取る");

	AddField("nameFlowTime", nameFlowTime)
		.Category("StarName")
		.Tooltip("左端から右端まで通り過ぎるのにかける秒数");

	AddField("nameFlowPower", nameFlowPower)
		.Category("StarName")
		.Tooltip("中央での粘り。1 で等速、大きいほど中央がゆっくりになる。3 で全体の約6割を中央付近に使う");
}

// パス
CalyxEngine::ParamPath MeteoriteForecast::ForecastParam::GetParamPath() const {
	return { CalyxEngine::ParamDomain::Game, "MeteoriteForecast", "Actor/Meteorite" };
}
