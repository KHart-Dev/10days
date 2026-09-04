#include "MeteoriteForecast.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include <Game/Result/ResultCarry.h>

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
}

MeteoriteForecast::MeteoriteForecast()
	: Actor("plane.obj", "MeteoriteForecast") {}

void MeteoriteForecast::Initialize() {
	Actor::Initialize();

	param_.LoadParams();

	DisableGravity();

	// ステージ番号は次ステージへ移る前に ResultCarry へ入る運用。
	stageIndex_ = ResultCarry::stageIndex;
	ApplyStageTexture();

	// ちょっと空中投影っぽい感じになるように
	SetBlendMode(BlendMode::ADD);
	SetLightingMode(LightingMode::NoLighting);
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

	UpdatePhase(dt);

	if (phase_ != Phase::Hidden) {
		UpdateVisual(dt);
		FollowCamera();
	}

	Actor::Update(dt);
}

void MeteoriteForecast::SetStage(int stageIndex) {

	stageIndex_ = stageIndex < 0 ? 0 : stageIndex;
	ApplyStageTexture();
}

void MeteoriteForecast::ApplyStageTexture() {

	SetTexture(kForecastTexturePrefix + std::to_string(stageIndex_) + kForecastTextureSuffix);
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
	SetColor(color);
}

void MeteoriteForecast::FollowCamera() {

	Camera3d* camera = CameraManager::GetMain3d();
	if (!camera) {
		return;
	}

	// カメラの軸
	const CalyxEngine::Matrix4x4& cameraMatrix = camera->GetWorldTransform().matrix.world;
	// カメラやその親に拡大率が入っていると軸の長さが 1 でなくなり、
	// distance も offset も倍率ぶんずれる。正規化してから使う。
	const Vector3 right = Vector3{ cameraMatrix.m[0][0], cameraMatrix.m[0][1], cameraMatrix.m[0][2] }.Normalize();
	const Vector3 up = Vector3{ cameraMatrix.m[1][0], cameraMatrix.m[1][1], cameraMatrix.m[1][2] }.Normalize();
	const Vector3 forward = Vector3{ cameraMatrix.m[2][0], cameraMatrix.m[2][1], cameraMatrix.m[2][2] }.Normalize();
	const Vector3 eye = { cameraMatrix.m[3][0], cameraMatrix.m[3][1], cameraMatrix.m[3][2] };

	// 揺れ幅は板の高さに対する割合で持つ。
	// メートルで持つと distance を変えたときだけ画面上の揺れ幅が変わってしまう。
	const float bob = std::sin(animeTime_ * param_.bobSpeed) * param_.bobRate * param_.size.y;

	auto& wt = GetWorldTransform();
	wt.translation =
		eye
		+ forward * param_.distance
		+ right * param_.offset.x
		+ up * (param_.offset.y + bob);

	// カメラと同じ向きにする
	const Quaternion cameraRotation = Quaternion::FromMatrix(cameraMatrix);
	const Quaternion rotationOffset = Quaternion::EulerToQuaternion({
		Deg2Rad(param_.rotationOffsetDeg.x),
		Deg2Rad(param_.rotationOffsetDeg.y),
		Deg2Rad(param_.rotationOffsetDeg.z) });

	wt.rotation = Quaternion::Multiply(rotationOffset, cameraRotation);
	wt.rotationSource = RotationSource::Quaternion;
}

void MeteoriteForecast::Open(Mode mode) {

	mode_ = mode;
	phase_ = Phase::Opening;
	SetDrawEnable(true);
}

void MeteoriteForecast::Close() {
	phase_ = Phase::Closing;
}

bool MeteoriteForecast::IsClosePressed() {
	return CalyxFoundation::Input::TriggerKey(kCloseKey)
		|| CalyxFoundation::Input::TriggerGamepadButton(kCloseButton);
}

bool MeteoriteForecast::IsPeekHeld() {
	return CalyxFoundation::Input::PushKey(kPeekKey)
		|| CalyxFoundation::Input::PushGamepadButton(kPeekButton);
}

void MeteoriteForecast::DisableGravity() {
	auto& movement = GetCharacterMovement();
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
}

// パス
CalyxEngine::ParamPath MeteoriteForecast::ForecastParam::GetParamPath() const {
	return { CalyxEngine::ParamDomain::Game, "MeteoriteForecast", "Actor/Meteorite" };
}
