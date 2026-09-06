#pragma once
// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// std
#include <cstdint>
#include <memory>
#include <vector>

class UiSprite;

CALYX_OBJECT(Category = GameObject, DisplayName = "TitleUIManager", Icon = "Textures/white1x1.png")
class TitleUIManager : public Actor {
public:
	TitleUIManager();
	~TitleUIManager() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void DerivativeGui() override;

private:

	// 1280x720 基準
	static constexpr float kScreenWidth = 1280.0f;
	static constexpr float kScreenHeight = 720.0f;

	static constexpr int32_t kMaxFloaterCount = 32;

	// 漂うFloater1体ぶんの状態。
	// 位置はここで持ち、毎フレームSpriteへ書き出す。
	struct FloaterMotion {
		std::shared_ptr<UiSprite> sprite;

		float x = 0.0f;
		float baseY = 0.0f;
		float size = 120.0f;

		float driftSpeed = 0.0f;   // 横に流れる速さ(px/sec)。符号が進む向き
		float bobAmplitude = 0.0f; // 上下の揺れ幅(px)
		float bobOmega = 0.0f;     // 上下の揺れの角速度(rad/sec)
		float bobPhase = 0.0f;     // 揺れの位相。個体ごとにずらす
		float spinSpeed = 0.0f;    // 回転の速さ(deg/sec)
		float rotationDeg = 0.0f;
	};

	// OptionParam と同じ形式で調整値を保存する
	struct TitleUIParam : CalyxEngine::SerializableObject {
		TitleUIParam() {
			AddField("titleLogoCenterX", titleLogoCenterX)
				.Category("TitleLogo")
				.Tooltip("Titleロゴの中心X座標(px)");

			AddField("titleLogoWidth", titleLogoWidth)
				.Category("TitleLogo")
				.Tooltip("Titleロゴの横サイズ(px)");

			AddField("titleLogoHeight", titleLogoHeight)
				.Category("TitleLogo")
				.Tooltip("Titleロゴの縦サイズ(px)");

			AddField("titleGapFromLogo", titleGapFromLogo)
				.Category("TitleLogo")
				.Tooltip("Titleロゴの下端からTitleBridgeの上端までの間隔(px)");

			AddField("titleBridgeCenterX", titleBridgeCenterX)
				.Category("TitleBridge")
				.Tooltip("TitleBridgeの中心X座標(px)");

			AddField("titleBridgeCenterY", titleBridgeCenterY)
				.Category("TitleBridge")
				.Tooltip("TitleBridgeの中心Y座標(px)");

			AddField("titleBridgeWidth", titleBridgeWidth)
				.Category("TitleBridge")
				.Tooltip("TitleBridgeの横サイズ(px)");

			AddField("titleBridgeHeight", titleBridgeHeight)
				.Category("TitleBridge")
				.Tooltip("TitleBridgeの縦サイズ(px)");

			AddField("titleABCenterX", titleABCenterX)
				.Category("TitleAB")
				.Tooltip("TitleA / TitleB 共通の中心X座標(px)");

			AddField("titleGapFromBridge", titleGapFromBridge)
				.Category("TitleAB")
				.Tooltip("TitleBridgeの下端からTitleAの上端までの間隔(px)");

			AddField("titleAWidth", titleAWidth)
				.Category("TitleAB")
				.Tooltip("TitleAの横サイズ(px)");

			AddField("titleAHeight", titleAHeight)
				.Category("TitleAB")
				.Tooltip("TitleAの縦サイズ(px)");

			AddField("titleBWidth", titleBWidth)
				.Category("TitleAB")
				.Tooltip("TitleBの横サイズ(px)");

			AddField("titleBHeight", titleBHeight)
				.Category("TitleAB")
				.Tooltip("TitleBの縦サイズ(px)");

			AddField("titleABGap", titleABGap)
				.Category("TitleAB")
				.Tooltip("TitleAの下端とTitleBの上端の間隔(px)。0で詰めて並ぶ");

			AddField("planetOffsetX", planetOffsetX)
				.Category("Planet")
				.Tooltip("画面端から内側へ押し込む量(px)。0でちょうど半分はみ出す");

			AddField("planetLeftSize", planetLeftSize)
				.Category("Planet")
				.Tooltip("左のPlanetの直径(px)");

			AddField("planetLeftCenterY", planetLeftCenterY)
				.Category("Planet")
				.Tooltip("左のPlanetの中心Y座標(px)");

			AddField("planetRightSize", planetRightSize)
				.Category("Planet")
				.Tooltip("右のPlanetの直径(px)");

			AddField("planetRightCenterY", planetRightCenterY)
				.Category("Planet")
				.Tooltip("右のPlanetの中心Y座標(px)");

			AddField("titleOrderInLayer", titleOrderInLayer)
				.Category("Sorting")
				.Tooltip("Bridge / A / B のOrderInLayer");

			AddField("planetOrderInLayer", planetOrderInLayer)
				.Category("Sorting")
				.Tooltip("PlanetのOrderInLayer。一番奥にするならFloaterより小さくする");

			AddField("floaterOrderInLayer", floaterOrderInLayer)
				.Category("Sorting")
				.Tooltip("FloaterのOrderInLayer。タイトルより小さい値にして後ろへ回す");

			AddField("floaterCount", floaterCount)
				.Category("Floater")
				.Tooltip("後ろで漂わせるFloaterの数")
				.Range(0.0f, static_cast<float>(kMaxFloaterCount));

			AddField("floaterAlpha", floaterAlpha)
				.Category("Floater")
				.Tooltip("Floaterの不透明度。下げるほど奥に見える")
				.Range(0.0f, 1.0f);

			AddField("floaterBrightness", floaterBrightness)
				.Category("Floater")
				.Tooltip("Floaterの明るさ。1.0が通常、下げるほど奥に沈む")
				.Range(0.0f, 1.0f);

			AddField("floaterSizeMin", floaterSizeMin)
				.Category("Floater")
				.Tooltip("Floaterの1辺サイズの最小値(px)");

			AddField("floaterSizeMax", floaterSizeMax)
				.Category("Floater")
				.Tooltip("Floaterの1辺サイズの最大値(px)");

			AddField("floaterTopY", floaterTopY)
				.Category("Floater")
				.Tooltip("Floaterが漂う範囲の上端Y(px)");

			AddField("floaterBottomY", floaterBottomY)
				.Category("Floater")
				.Tooltip("Floaterが漂う範囲の下端Y(px)");

			AddField("floaterMargin", floaterMargin)
				.Category("Floater Motion")
				.Tooltip("画面外どれだけ出たら反対側へ回り込ませるか(px)");

			AddField("floaterSpeedMin", floaterSpeedMin)
				.Category("Floater Motion")
				.Tooltip("横に流れる速さの最小値(px/sec)");

			AddField("floaterSpeedMax", floaterSpeedMax)
				.Category("Floater Motion")
				.Tooltip("横に流れる速さの最大値(px/sec)");

			AddField("floaterBobAmplitudeMin", floaterBobAmplitudeMin)
				.Category("Floater Motion")
				.Tooltip("上下の揺れ幅の最小値(px)");

			AddField("floaterBobAmplitudeMax", floaterBobAmplitudeMax)
				.Category("Floater Motion")
				.Tooltip("上下の揺れ幅の最大値(px)");

			AddField("floaterBobCycleMin", floaterBobCycleMin)
				.Category("Floater Motion")
				.Tooltip("上下に1往復する時間の最小値(sec)");

			AddField("floaterBobCycleMax", floaterBobCycleMax)
				.Category("Floater Motion")
				.Tooltip("上下に1往復する時間の最大値(sec)");

			AddField("floaterSpinSpeedMin", floaterSpinSpeedMin)
				.Category("Floater Motion")
				.Tooltip("回転の速さの最小値(deg/sec)");

			AddField("floaterSpinSpeedMax", floaterSpinSpeedMax)
				.Category("Floater Motion")
				.Tooltip("回転の速さの最大値(deg/sec)");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {
				CalyxEngine::ParamDomain::Game,
				"TitleUIParam",
				"Actor/TitleUIManager/TitleUIParam"
			};
		}

		// TitleLogo
		// Bridgeの上端から上へ積むので、中心Yは持たずBridgeから逆算する。
		// Title.pngは1000x400なので5:2で拡縮する。
		float titleLogoCenterX = 640.0f;
		float titleLogoWidth = 400.0f;
		float titleLogoHeight = 160.0f;
		float titleGapFromLogo = 16.0f;

		// TitleBridge
		// ロゴを上に載せたぶん、Bridge以下はまとめて下へずらしてある。
		float titleBridgeCenterX = 640.0f;
		float titleBridgeCenterY = 300.0f;
		float titleBridgeWidth = 800.0f;
		float titleBridgeHeight = 200.0f;

		// TitleA / TitleB
		// Bridgeの下に間をあけ、AとBはYを詰めて縦に並べる。
		float titleABCenterX = 640.0f;
		float titleGapFromBridge = 60.0f;
		float titleAWidth = 360.0f;
		float titleAHeight = 120.0f;
		float titleBWidth = 360.0f;
		float titleBHeight = 120.0f;
		float titleABGap = 0.0f;

		// Planet
		// 左右それぞれ画面端に中心を置いて、半分を画面外へはみ出させる。
		float planetOffsetX = 0.0f;
		float planetLeftSize = 420.0f;
		float planetLeftCenterY = 430.0f;
		float planetRightSize = 340.0f;
		float planetRightCenterY = 250.0f;

		// Sprite描画順
		// 同一SortingLayer内で、奥から Planet → Floater → タイトル の順に重ねる。
		int32_t titleOrderInLayer = 100;
		int32_t floaterOrderInLayer = 10;
		int32_t planetOrderInLayer = 5;

		// Floaterの見た目
		int32_t floaterCount = 6;
		float floaterAlpha = 1.0f;
		float floaterBrightness = 0.75f;
		float floaterSizeMin = 90.0f;
		float floaterSizeMax = 150.0f;
		float floaterTopY = 90.0f;
		float floaterBottomY = 630.0f;

		// Floaterの動き
		float floaterMargin = 140.0f;
		float floaterSpeedMin = 14.0f;
		float floaterSpeedMax = 46.0f;
		float floaterBobAmplitudeMin = 10.0f;
		float floaterBobAmplitudeMax = 34.0f;
		float floaterBobCycleMin = 2.6f;
		float floaterBobCycleMax = 5.4f;
		float floaterSpinSpeedMin = -18.0f;
		float floaterSpinSpeedMax = 18.0f;
	};

private:

	void InitializeSprites();
	void UpdateInput();
	void ApplyTitleLayout();
	void ApplyPlanetLayout();

	void RebuildFloaters();
	void ResetFloater(FloaterMotion& motion, bool randomizeX);
	void ApplyFloaterStaticParams();
	void UpdateFloaters(float dt);

	void DisableGravity();

	static float PickInRange(float a, float b);

private:

	TitleUIParam param_;

	std::shared_ptr<UiSprite> titleLogo_;
	std::shared_ptr<UiSprite> titleBridge_;
	std::shared_ptr<UiSprite> titleA_;
	std::shared_ptr<UiSprite> titleB_;
	std::shared_ptr<UiSprite> planetLeft_;
	std::shared_ptr<UiSprite> planetRight_;
	std::vector<FloaterMotion> floaters_;

	bool spritesReady_ = false;

	// シーン遷移を要求済みかどうか。実際に切り替わるまで数フレームあるので、
	// その間にAを連打されても2回要求しないようにする。
	bool transitionRequested_ = false;
};
