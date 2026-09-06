#include "BackGroundObjects.h"

// engine
#include <Engine/Objects/3D/Actor/StaticModelObject.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"

// std
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	constexpr float kPitch = std::numbers::pi_v<float> *0.5f;

	constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

	// 増やすときは BackGroundObjects::kKindCount と速さの項目も足す。
	constexpr const char* kBackGroundTextures[] = {
		"Textures/BackGround/background01.png",
		"Textures/BackGround/background02.png",
		"Textures/BackGround/background03.png",
	};
}

BackGroundObjects::BackGroundObjects()
	: Actor("debugCube.obj", "BackGroundObjects") {

}

void BackGroundObjects::Initialize() {
	Actor::Initialize();

	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();

	DisableGravity();
	Actor::SetDrawEnable(false);

	if (IsTransient()) {
		return;
	}

	ready_ = true;
}

void BackGroundObjects::Update(float dt) {

	// 初回はここで全部生成される
	RebuildObjects();
	ApplyStaticParams();
	UpdateMotions(dt);
}

/////////////////////////////////////////////////////////////////////////////////
//		枚数の増減
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::RebuildObjects() {

	if (!ready_) {
		return;
	}

	const int32_t clamped =
		std::clamp(param_.objectCount, 0, kMaxObjectCount);
	const size_t requiredCount = static_cast<size_t>(clamped);

	// 足りない分だけ生成
	while (objects_.size() < requiredCount) {
		BackGroundMotion motion{};

		motion.object =
			SceneAPI::Instantiate<StaticModelObject>("plane.obj", "BackGroundObject");

		if (motion.object) {
			motion.object->Initialize();

			motion.object->SetLightingMode(LightingMode::NoLighting);
			motion.object->SetBlendMode(BlendMode::NORMAL);
			motion.object->SetBillboardMode(BillboardMode::Full);
			motion.object->SetCastShadow(false);
		}

		ResetMotion(motion, true);

		objects_.push_back(std::move(motion));
	}

	// 多すぎる場合
	while (objects_.size() > requiredCount) {

		// Destroyでシーン側へ破棄を通知してから、所有リストから取り除く
		if (objects_.back().object) {
			objects_.back().object->Destroy();
		}

		objects_.pop_back();
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		1枚ぶんの抽選
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::ResetMotion(BackGroundMotion& motion, bool randomizeTravel) {

	// BackGroundに入っている種類からランダムに選ぶ
	motion.kind = Random::Generate<int32_t>(0, kKindCount - 1);

	if (motion.object) {
		motion.object->SetTexture(kBackGroundTextures[motion.kind]);
	}

	motion.size = PickInRange(param_.sizeMin, param_.sizeMax);

	const float spread = std::fabsf(param_.spreadHalfWidth);
	motion.side = Random::Generate(-spread, spread);

	const float depthRandom = std::fabsf(param_.depthRandom);
	motion.depth = Random::Generate(-depthRandom, depthRandom);

	// 全部が完全に平行だと流れが揃いすぎるので、1枚ずつ少しだけ傾ける
	const float angleDeg = std::clamp(param_.flowAngleRandomDeg, 0.0f, 90.0f);
	motion.angleRad = Random::Generate(-angleDeg, angleDeg) * kDegToRad;

	// 種類ごとの速さを基準にして、少しだけばらす
	const float baseSpeed = SpeedOfKind(motion.kind);
	const float jitter = std::clamp(param_.speedJitter, 0.0f, 1.0f);
	motion.speed = baseSpeed * Random::Generate(1.0f - jitter, 1.0f + jitter);

	const float halfLength = std::fabsf(param_.travelHalfLength);

	if (randomizeTravel) {
		motion.travel = Random::Generate(-halfLength, halfLength);
	} else {
		// 流れてきた側へ戻す
		motion.travel = -halfLength;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		見た目(毎フレーム反映)
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::ApplyStaticParams() {

	const float bright = std::clamp(param_.brightness, 0.0f, 1.0f);
	const float a = std::clamp(param_.alpha, 0.0f, 1.0f);

	// plane.obj は素の状態でXY平面に立っている。
	// XZ平面を流すときだけ、X+90度で寝かせて見下ろしカメラに正対させる。
	const CalyxEngine::Vector3 rotation = param_.useXYPlane
		? CalyxEngine::Vector3(0.0f, 0.0f, 0.0f)
		: CalyxEngine::Vector3(kPitch, 0.0f, 0.0f);

	for (BackGroundMotion& motion : objects_) {
		if (!motion.object) {
			continue;
		}

		motion.object->SetRotate(rotation);
		motion.object->SetScale(
			CalyxEngine::Vector3(motion.size, motion.size, 1.0f)
		);
		motion.object->SetColor(
			CalyxEngine::Vector4(bright, bright, bright, a)
		);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		移動
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::UpdateMotions(float dt) {

	CalyxEngine::Vector3 flow{};
	CalyxEngine::Vector3 side{};
	CalyxEngine::Vector3 depth{};
	GetFlowAxis(flow, side, depth);

	// 帯の中心はこのActorの位置。Groundより下へ置けばステージの後ろを流れる。
	const CalyxEngine::Vector3 origin = GetWorldTransform().GetWorldPosition();

	const float halfLength = std::fabsf(param_.travelHalfLength);

	for (BackGroundMotion& motion : objects_) {

		motion.travel += motion.speed * dt;

		// 端まで流れたら反対側へワープして、種類と速さを引き直す
		if (motion.travel > halfLength) {
			ResetMotion(motion, false);
		}

		if (!motion.object) {
			continue;
		}

		// 基準の向きを、動く面の中で angleRad だけ回して1枚ぶんの向きにする。
		// flow と side は面の正規直交基底なので、この2本で回せば面から外れない。
		const float cos = std::cosf(motion.angleRad);
		const float sin = std::sinf(motion.angleRad);

		const CalyxEngine::Vector3 myFlow = flow * cos + side * sin;
		const CalyxEngine::Vector3 mySide = side * cos - flow * sin;

		const CalyxEngine::Vector3 offset =
			myFlow * motion.travel
			+ mySide * motion.side
			+ depth * motion.depth;

		motion.object->SetTranslate(origin + offset);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		流れる向き
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::GetFlowAxis(
	CalyxEngine::Vector3& outFlow,
	CalyxEngine::Vector3& outSide,
	CalyxEngine::Vector3& outDepth
) const {

	// 面から外れる成分は depthRandom で別に散らすので、向きからは落とす。
	CalyxEngine::Vector3 flow = param_.flowDirection;

	if (param_.useXYPlane) {

		// Title用。カメラに正対したXY平面を流れる
		flow.z = 0.0f;

		if (flow.x * flow.x + flow.y * flow.y < 1e-6f) {
			// 0ベクトルを入れられても止まらないよう、既定の右上→左下に戻す
			flow = CalyxEngine::Vector3(-1.0f, -1.0f, 0.0f);
		}

		outFlow = flow.Normalize();

		// 流れる向きをZ軸まわりに90度回した向きが帯の幅方向
		outSide = CalyxEngine::Vector3(-outFlow.y, outFlow.x, 0.0f);
		outDepth = CalyxEngine::Vector3(0.0f, 0.0f, 1.0f);
		return;
	}

	// ステージ用。見下ろしカメラなのでXZ平面を流れる
	flow.y = 0.0f;

	if (flow.x * flow.x + flow.z * flow.z < 1e-6f) {
		flow = CalyxEngine::Vector3(-1.0f, 0.0f, -1.0f);
	}

	outFlow = flow.Normalize();

	// 流れる向きをY軸まわりに90度回した向きが帯の幅方向
	outSide = CalyxEngine::Vector3(-outFlow.z, 0.0f, outFlow.x);
	outDepth = CalyxEngine::Vector3(0.0f, 1.0f, 0.0f);
}

float BackGroundObjects::SpeedOfKind(int32_t kind) const {

	switch (kind) {
	case 0:  return param_.speed01;
	case 1:  return param_.speed02;
	case 2:  return param_.speed03;
	default: return param_.speed01;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		ユーティリティ
/////////////////////////////////////////////////////////////////////////////////
float BackGroundObjects::PickInRange(float a, float b) {

	// Inspectorで min と max が入れ替わっていても抽選できるようにする。
	const float low = a < b ? a : b;
	const float high = a < b ? b : a;

	return Random::Generate(low, high);
}

void BackGroundObjects::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

/////////////////////////////////////////////////////////////////////////////////
//		デバッグgui
/////////////////////////////////////////////////////////////////////////////////
void BackGroundObjects::DerivativeGui() {

	using namespace GuiCmd;

	if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		PropertyText("Objects", "%d", static_cast<int>(objects_.size()));

		param_.ShowGui();

		EndSection();
	}
}
