#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

// std
#include <string_view>

/// <summary>
/// 隕石の天気予報。カメラの手前に板を1枚出して空中投影のように見せる。
/// </summary>
CALYX_OBJECT(Category = GameObject, DisplayName = "MeteoriteForecast", Icon = "UI/Tool/event.png"
	, Placeable = false, PrefabEditable = true, PrefabRoot = true)
class MeteoriteForecast : public Actor {

public:

	MeteoriteForecast();
	~MeteoriteForecast() override = default;

	void Initialize() override;
	void Update(float dt) override;

	/// ゲーム開始時の表示を始める。閉じる入力が来るまで出したまま
	void ShowAtStart();

	/// 表示するステージを切り替える
	void SetStage(int stageIndex);

	/// スタート時の予報を見せているか
	bool IsWaitingAtStart() const;

	/// 何かしら映っているか
	bool IsVisible() const { return phase_ != Phase::Hidden; }

	std::string_view GetObjectClassName() const override { return "MeteoriteForecast"; }

private:

	enum class Phase {
		Hidden,		//< 消えている
		Opening,	//< 起動アニメ中
		Shown,		//< 開ききっている
		Closing,	//< 畳んでいる
	};

	/// 何のきっかけで出ているか。閉じ方が変わる
	enum class Mode {
		Start,		//< ゲーム開始時。閉じるボタンを押すまで出たまま
		Peek,		//< ゲーム中。覗きボタンを押している間だけ
	};

	void UpdatePhase(float dt);
	void UpdateVisual(float dt);
	void FollowCamera();

	void Open(Mode mode);
	void Close();

	void DisableGravity();

	/// stageIndex_ から絵を読み直す
	void ApplyStageTexture();

	static bool IsClosePressed();
	static bool IsPeekHeld();

	Phase phase_ = Phase::Hidden;
	Mode  mode_ = Mode::Start;

	int stageIndex_ = 0;

	/// 0 = 畳んだ状態、1 = 開ききった状態
	float openRate_ = 0.0f;
	/// 明滅と浮遊のために回し続ける時間
	float animeTime_ = 0.0f;

	struct ForecastParam : CalyxEngine::SerializableObject {
		ForecastParam();

		CalyxEngine::ParamPath GetParamPath() const override;

		float distance = 6.0f;						//< カメラから板までの距離
		CalyxEngine::Vector2 size{ 5.0f, 3.0f };	//< 板の大きさ (m)
		CalyxEngine::Vector2 offset{ 0.0f, 0.0f };	//< 画面内のずらし。x=右, y=上
		CalyxEngine::Vector3 rotationOffsetDeg{};	//< 板が裏返るときの微調整

		float openTime = 0.35f;						//< 展開にかける秒数
		float closeTime = 0.18f;					//< 畳むのにかける秒数

		CalyxEngine::Vector4 color{ 0.45f, 0.85f, 1.0f, 1.0f };
		float flickerAmp = 0.12f;					//< 明滅の強さ
		float flickerSpeed = 22.0f;					//< 明滅の速さ
		float bobRate = 0.004f;						//< 上下の揺れ幅。板の高さに対する割合
		float bobSpeed = 1.6f;						//< 上下の揺れの速さ
	};

	ForecastParam param_;

public:

	void DerivativeGui() override;

};
