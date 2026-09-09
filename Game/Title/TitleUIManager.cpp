#include "TitleUIManager.h"

// engine
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"

// game
#include <Game/UI/UiSprite.h>
#include <Game/Scene/SceneFlow.h>
#include <Game/Audio/GameAudio.h>

// std
#include <algorithm>
#include <cmath>
#include <numbers>
#include <nlohmann/json.hpp>

namespace {
	constexpr const char* kTitleTexture =
		"Textures/Title/Title.png";
	constexpr const char* kTitleBridgeTexture =
		"Textures/Title/titleBridge.png";

	constexpr const char* kTitleATexture =
		"Textures/Title/titleA.png";
	constexpr const char* kTitleBTexture =
		"Textures/Title/titleB.png";

	constexpr const char* kPlanetLeftTexture =
		"Textures/planet/planet.png";
	constexpr const char* kPlanetRightTexture =
		"Textures/planet/planet.png";

	constexpr const char* kFloaterTexture =
		"Textures/human/human.png";
}


TitleUIManager::TitleUIManager()
	: Actor("debugCube.obj", "TitleUIManager") {

	// OptionManager と同じように保存済みパラメータを読み込む
	param_.LoadParams();
}

void TitleUIManager::Initialize() {
	Actor::Initialize();

	DisableGravity();
	Actor::SetDrawEnable(false);

	// プレハブ用の一時コピーからSpriteを撒かない
	if (IsTransient()) {
		return;
	}

	GameAudio::PlayBgm(GameAudio::kBgmTitle);
	InitializeSprites();
}

void TitleUIManager::Update(float dt) {

	UpdateInput();

	// Inspectorで値を変更したときに、その場で配置を確認できるようにする。
	ApplyTitleLayout();
	ApplyPlanetLayout();

	RebuildFloaters();
	ApplyFloaterStaticParams();
	UpdateFloaters(dt);
}

/////////////////////////////////////////////////////////////////////////////////
//		入力
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::UpdateInput() {

	// 遷移要求は1回だけ。要求後は入力を受け付けない。
	if (transitionRequested_) {
		return;
	}

	// A / Space : AnimationSceneへ
	if (CalyxFoundation::Input::TriggerGamepadButton(
		CalyxFoundation::PadButton::A)/* ||
		CalyxFoundation::Input::TriggerKey(DIK_SPACE)*/) {

		transitionRequested_ = true;
		GameAudio::PlaySe(GameAudio::kSeStart);
		SceneFlow::GoToAnimation();
		//SceneFlow::StartStage(0);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Sprite生成
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::InitializeSprites() {

	titleLogo_ = SceneAPI::Instantiate<UiSprite>(kTitleTexture);
	titleBridge_ = SceneAPI::Instantiate<UiSprite>(kTitleBridgeTexture);
	titleA_ = SceneAPI::Instantiate<UiSprite>(kTitleATexture);
	titleB_ = SceneAPI::Instantiate<UiSprite>(kTitleBTexture);

	planetLeft_ = SceneAPI::Instantiate<UiSprite>(kPlanetLeftTexture);
	planetRight_ = SceneAPI::Instantiate<UiSprite>(kPlanetRightTexture);

	// 中心基準にしておくと、上下に積むときも画面端に半分だけ出すときも
	// 計算がサイズの半分ぶんで済む。
	for (const std::shared_ptr<UiSprite>& sprite :
		{ titleLogo_, titleBridge_, titleA_, titleB_, planetLeft_, planetRight_ }) {
		if (!sprite) {
			continue;
		}
		sprite->SetAnchor({ 0.5f, 0.5f });
		sprite->SetRotationDeg(0.0f);
		sprite->SetColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	}

	spritesReady_ = true;

	ApplyTitleLayout();
	ApplyPlanetLayout();

	RebuildFloaters();
	ApplyFloaterStaticParams();
}

/////////////////////////////////////////////////////////////////////////////////
//		タイトル3枚の配置
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::ApplyTitleLayout() {

	if (!spritesReady_) {
		return;
	}

	// Titleロゴ。Bridgeの上端から上へ積むので、中心Yはここで逆算する。
	const float bridgeTop =
		param_.titleBridgeCenterY - param_.titleBridgeHeight * 0.5f;

	const float titleLogoBottom = bridgeTop - param_.titleGapFromLogo;
	const float titleLogoCenterY = titleLogoBottom - param_.titleLogoHeight * 0.5f;

	if (titleLogo_) {
		titleLogo_->SetSizePx(param_.titleLogoWidth, param_.titleLogoHeight);
		titleLogo_->SetPositionPx(param_.titleLogoCenterX, titleLogoCenterY);
		titleLogo_->SetOrderInLayer(param_.titleOrderInLayer);
	}

	// TitleBridge。ここが基準になる。
	if (titleBridge_) {
		titleBridge_->SetSizePx(
			param_.titleBridgeWidth,
			param_.titleBridgeHeight
		);
		titleBridge_->SetPositionPx(
			param_.titleBridgeCenterX,
			param_.titleBridgeCenterY
		);
		titleBridge_->SetOrderInLayer(param_.titleOrderInLayer);
	}

	// スクリーン座標はYが下向き。
	// Bridgeの下端から titleGapFromBridge だけ空けて TitleA の上端が来る。
	const float bridgeBottom =
		param_.titleBridgeCenterY + param_.titleBridgeHeight * 0.5f;

	const float titleATop = bridgeBottom + param_.titleGapFromBridge;
	const float titleACenterY = titleATop + param_.titleAHeight * 0.5f;

	// TitleB は TitleA の下端にそのまま続ける(titleABGap の既定値は0)。
	const float titleBTop =
		titleATop + param_.titleAHeight + param_.titleABGap;
	const float titleBCenterY = titleBTop + param_.titleBHeight * 0.5f;

	if (titleA_) {
		titleA_->SetSizePx(param_.titleAWidth, param_.titleAHeight);
		titleA_->SetPositionPx(param_.titleABCenterX, titleACenterY);
		titleA_->SetOrderInLayer(param_.titleOrderInLayer);
	}

	if (titleB_) {
		titleB_->SetSizePx(param_.titleBWidth, param_.titleBHeight);
		titleB_->SetPositionPx(param_.titleABCenterX, titleBCenterY);
		titleB_->SetOrderInLayer(param_.titleOrderInLayer);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Planetの配置
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::ApplyPlanetLayout() {

	if (!spritesReady_) {
		return;
	}

	// アンカーが中心(0.5, 0.5)なので、中心を画面端そのものに置けば
	// 左右それぞれちょうど半分が画面外へ出る。
	// planetOffsetX を足すと内側へ寄って、はみ出す量が減る。
	const float leftX = param_.planetOffsetX;
	const float rightX = kScreenWidth - param_.planetOffsetX;

	if (planetLeft_) {
		planetLeft_->SetSizePx(param_.planetLeftSize, param_.planetLeftSize);
		planetLeft_->SetPositionPx(leftX, param_.planetLeftCenterY);
		planetLeft_->SetOrderInLayer(param_.planetOrderInLayer);
	}

	if (planetRight_) {
		planetRight_->SetSizePx(param_.planetRightSize, param_.planetRightSize);
		planetRight_->SetPositionPx(rightX, param_.planetRightCenterY);
		planetRight_->SetOrderInLayer(param_.planetOrderInLayer);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Floaterの増減
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::RebuildFloaters() {

	if (!spritesReady_) {
		return;
	}

	const int32_t clamped =
		std::clamp(param_.floaterCount, 0, kMaxFloaterCount);
	const size_t requiredCount = static_cast<size_t>(clamped);

	// 足りない分だけ生成
	while (floaters_.size() < requiredCount) {
		FloaterMotion motion{};
		motion.sprite = SceneAPI::Instantiate<UiSprite>(kFloaterTexture);

		if (motion.sprite) {
			motion.sprite->SetAnchor({ 0.5f, 0.5f });
		}

		// 起動直後から散らばっていてほしいのでXもランダムにする。
		ResetFloater(motion, true);

		floaters_.push_back(std::move(motion));
	}

	// 多すぎる場合
	while (floaters_.size() > requiredCount) {

		// Destroyでシーン側へ破棄を通知してから、所有リストから取り除く。
		if (floaters_.back().sprite) {
			floaters_.back().sprite->Destroy();
		}

		floaters_.pop_back();
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Floater1体の抽選
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::ResetFloater(FloaterMotion& motion, bool randomizeX) {

	motion.size = PickInRange(param_.floaterSizeMin, param_.floaterSizeMax);

	// 上下の漂う範囲。上端と下端が逆に入っていても動くようにしておく。
	motion.baseY = PickInRange(param_.floaterTopY, param_.floaterBottomY);

	const float speed = PickInRange(param_.floaterSpeedMin, param_.floaterSpeedMax);
	const bool toRight = Random::Generate(0.0f, 1.0f) < 0.5f;
	motion.driftSpeed = toRight ? speed : -speed;

	motion.bobAmplitude = PickInRange(
		param_.floaterBobAmplitudeMin,
		param_.floaterBobAmplitudeMax
	);

	// 周期(sec)から角速度(rad/sec)へ。0除算を避けるため下限を入れる。
	const float cycle = std::fmax(
		PickInRange(param_.floaterBobCycleMin, param_.floaterBobCycleMax),
		0.01f
	);
	motion.bobOmega = 2.0f * std::numbers::pi_v<float> / cycle;
	motion.bobPhase = Random::Generate(0.0f, 2.0f * std::numbers::pi_v<float>);

	motion.spinSpeed = PickInRange(
		param_.floaterSpinSpeedMin,
		param_.floaterSpinSpeedMax
	);
	motion.rotationDeg = Random::Generate(0.0f, 360.0f);

	const float margin = std::fmax(param_.floaterMargin, 0.0f);
	const float half = motion.size * 0.5f;

	if (randomizeX) {
		motion.x = Random::Generate(-half, kScreenWidth + half);
	} else {
		// 進んでいる向きと反対側の画面外から入れ直す。
		motion.x = motion.driftSpeed >= 0.0f
			? -(half + margin)
			: kScreenWidth + half + margin;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Floaterの見た目(毎フレーム反映)
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::ApplyFloaterStaticParams() {

	// Floaterはタイトルより奥にいる想定なので、
	// OrderInLayerで後ろへ回したうえで暗く薄くする。
	const float brightness = std::clamp(param_.floaterBrightness, 0.0f, 1.0f);
	const float alpha = std::clamp(param_.floaterAlpha, 0.0f, 1.0f);

	for (FloaterMotion& motion : floaters_) {
		if (!motion.sprite) {
			continue;
		}

		motion.sprite->SetSizePx(motion.size, motion.size);
		motion.sprite->SetOrderInLayer(param_.floaterOrderInLayer);
		motion.sprite->SetColorRGBA(brightness, brightness, brightness, alpha);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		Floaterの移動
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::UpdateFloaters(float dt) {

	const float margin = std::fmax(param_.floaterMargin, 0.0f);
	const float twoPi = 2.0f * std::numbers::pi_v<float>;

	for (FloaterMotion& motion : floaters_) {

		motion.x += motion.driftSpeed * dt;

		motion.bobPhase += motion.bobOmega * dt;
		if (motion.bobPhase > twoPi) {
			motion.bobPhase -= twoPi;
		}

		motion.rotationDeg += motion.spinSpeed * dt;
		if (motion.rotationDeg > 360.0f) {
			motion.rotationDeg -= 360.0f;
		} else if (motion.rotationDeg < -360.0f) {
			motion.rotationDeg += 360.0f;
		}

		const float half = motion.size * 0.5f;

		// 画面外へ抜けきったら反対側から入れ直す。
		// このときだけ速さ・大きさ・高さを引き直すので、同じ並びが続かない。
		const bool goneRight =
			motion.driftSpeed >= 0.0f && motion.x > kScreenWidth + half + margin;
		const bool goneLeft =
			motion.driftSpeed < 0.0f && motion.x < -(half + margin);

		if (goneRight || goneLeft) {
			ResetFloater(motion, false);
			continue;
		}

		if (!motion.sprite) {
			continue;
		}

		const float y =
			motion.baseY + std::sinf(motion.bobPhase) * motion.bobAmplitude;

		motion.sprite->SetPositionPx(motion.x, y);
		motion.sprite->SetRotationDeg(motion.rotationDeg);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		ユーティリティ
/////////////////////////////////////////////////////////////////////////////////
float TitleUIManager::PickInRange(float a, float b) {

	// Inspectorで min と max が入れ替わっていても抽選できるようにする。
	const float low = a < b ? a : b;
	const float high = a < b ? b : a;

	return Random::Generate(low, high);
}

void TitleUIManager::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

/////////////////////////////////////////////////////////////////////////////////
//		デバッグgui
/////////////////////////////////////////////////////////////////////////////////
void TitleUIManager::DerivativeGui() {

	using namespace GuiCmd;

	if (BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		PropertyText("Floaters", "%d", static_cast<int>(floaters_.size()));

		// OptionManager と同じようにSerializableObjectの項目をInspectorへ表示
		param_.ShowGui();

		EndSection();
	}
}
