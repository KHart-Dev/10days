#pragma once

// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// game
#include <Game/Meteorite/InResult/MeteoriteWarning.h>

// std
#include <memory>
#include <vector>

/// <summary>リザルトシーンに1個置く。落下地点ぶんの予告円を子として持ち、号令を出す</summary>
CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "Meteorite Director", Icon = "UI/Tool/event.png")
class MeteoriteDirector : public Actor {

public:

	MeteoriteDirector();
	~MeteoriteDirector() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void AlwaysUpdate(float dt) override;

	/// 最終人数を出す合図用
	bool IsFinished() const;

private:

	/// 落下地点の上限。パラメータは配列を持てないので固定スロットで持つ
	static constexpr int kMaxSpots = 8;

	/// count に合わせて子の予告円を増減する
	void RebuildWarnings(int count);

	/// 子の予告円の位置をパラメータ側へ書き戻す
	void SyncSpotPositions();

	/// 子の予告円に号令を出す
	void StartAll();

	/// ステージを切り替えてパラメータを読み直し、予告円を生やし直す
	void LoadStage(int stageIndex);

	void DisableGravity();

	std::vector<std::shared_ptr<MeteoriteWarning>> warnings_;

	float timer_ = 0.0f;
	bool started_ = false;

	// GUI から要求されたステージ番号。-1 は要求なし。
	// DerivativeGui から直に読み直すとインスペクター描画中に子を Destroy することに
	// なるため、AlwaysUpdate まで持ち越す。
	int pendingStage_ = -1;

	struct MeteoriteDirectorParam : CalyxEngine::SerializableObject {
		MeteoriteDirectorParam();

		// どのステージのファイルを読むかを決める値。保存される中身ではないので
		// AddField しないこと。登録すると json 側の値が選択を上書きしてしまう。
		int stageIndex_ = 0;
		CalyxEngine::ParamPath GetParamPath() const override;

		float startDelay = 1.0f;

		MeteoriteFallSettings settings{};

		int                  count = 0;
		CalyxEngine::Vector3 pos[kMaxSpots]{};
		float                delay[kMaxSpots]{};
	};

	MeteoriteDirectorParam param_;

public:

	void DerivativeGui() override;

};
