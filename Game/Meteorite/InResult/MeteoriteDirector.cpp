#include "MeteoriteDirector.h"

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// game
#include <Game/Result/ResultCarry.h>

// std
#include <algorithm>
#include <string>

namespace {
	// Range() が縛るのは GUI の入力だけで、json から読んだ値は素通りする。
	// 固定スロットをはみ出さないよう、使う側で必ず丸める。
	int ClampSpotCount(int count, int maxCount) noexcept {
		// Windows.h の min/max マクロと衝突するため三項演算子で書く。
		const int lower = count < 0 ? 0 : count;
		return lower > maxCount ? maxCount : lower;
	}
}

MeteoriteDirector::MeteoriteDirector()
	: Actor("cone.obj", "MeteoriteDirector") {}

void MeteoriteDirector::Initialize() {
	if (IsTransient()) return;

	Actor::Initialize();

	// どのステージから来たかで落下地点が変わる。入れる人はゴールエリア待ち。
	LoadStage(ResultCarry::stageIndex);

	DisableGravity();
	SetDrawEnable(false);
}

void MeteoriteDirector::Update(float dt) {

	if (!started_) {
		timer_ += dt;
		if (timer_ >= param_.startDelay) {
			StartAll();
			started_ = true;
		}
	}

	Actor::Update(dt);
}

void MeteoriteDirector::AlwaysUpdate(float dt) {

	if (!IsTransient()) {

		// GUI からの切り替えはここまで持ち越して処理する
		if (pendingStage_ >= 0) {
			LoadStage(pendingStage_);
			pendingStage_ = -1;
		}

		param_.count = ClampSpotCount(param_.count, kMaxSpots);
		RebuildWarnings(param_.count);
		SyncSpotPositions();
	}

	Actor::AlwaysUpdate(dt);
}

void MeteoriteDirector::RebuildWarnings(int count) {

	const size_t requiredCount = static_cast<size_t>(count);

	// 足りない分だけ生成
	while (warnings_.size() < requiredCount) {

		// これから足すスロット番号
		const size_t index = warnings_.size();

		std::shared_ptr<MeteoriteWarning> warning =
			SceneAPI::Instantiate<MeteoriteWarning>();

		warning->Initialize();

		// 親の拡大率まで継承すると円が伸びるため、inheritScale は false。
		warning->SetParent(shared_from_this(), false);
		warning->GetWorldTransform().inheritRotate = false;
		warning->GetWorldTransform().translation = param_.pos[index];

		warnings_.push_back(std::move(warning));
	}

	// 多すぎる場合
	while (warnings_.size() > requiredCount) {

		// Destroy でシーン側へ破棄を通知してから、所有リストから取り除く。
		warnings_.back()->Destroy();

		warnings_.pop_back();
	}
}

void MeteoriteDirector::SyncSpotPositions() {

	for (size_t i = 0; i < warnings_.size(); i++) {
		if (warnings_[i]) {
			param_.pos[i] = warnings_[i]->GetWorldTransform().translation;
		}
	}
}

void MeteoriteDirector::StartAll() {

	// 落ちる順番はスロットごとの delay が決める
	for (size_t i = 0; i < warnings_.size(); i++) {
		if (warnings_[i]) {
			warnings_[i]->Start(param_.settings, param_.delay[i]);
		}
	}
}

void MeteoriteDirector::LoadStage(int stageIndex) {

	param_.stageIndex_ = stageIndex < 0 ? 0 : stageIndex;
	param_.LoadParams();

	// 位置の同期は「子 → 配列」の一方向なので、読み込んだ pos[] は生えている
	// 予告円には伝わらない。一度全部捨てて、次の RebuildWarnings で生成時の
	// 種付けを通して新しい位置に生え直させる。
	RebuildWarnings(0);
}

bool MeteoriteDirector::IsFinished() const {

	if (!started_) {
		return false;
	}

	return std::all_of(warnings_.begin(), warnings_.end(),
		[](const std::shared_ptr<MeteoriteWarning>& warning) {
			return !warning || warning->IsFinished();
		});
}

void MeteoriteDirector::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void MeteoriteDirector::DerivativeGui() {

	ImGui::Text("Warnings: %d", static_cast<int>(warnings_.size()));
	ImGui::Text("Started : %s", started_ ? "yes" : "no");
	ImGui::Text("Finished: %s", IsFinished() ? "yes" : "no");

	// ステージを行き来しながら落下地点を調整するためのデバッグ欄。
	// 本番の値は Initialize で ResultCarry::stageIndex から入る。
	int stage = param_.stageIndex_;
	if (ImGui::InputInt("Stage", &stage)) {
		pendingStage_ = stage < 0 ? 0 : stage;
	}

	param_.ShowGui();
}


// 登録
MeteoriteDirector::MeteoriteDirectorParam::MeteoriteDirectorParam() {

	AddField("startDelay", startDelay)
		.Category("Timing")
		.Tooltip("リザルトが始まってから号令を出すまでの秒数");

	AddField("blinkTimes", settings.blinkTimes)
		.Category("Warning")
		.Tooltip("何回点滅させてから落とすか");

	AddField("blinkPeriod", settings.blinkPeriod)
		.Category("Warning")
		.Tooltip("点滅1回ぶんの秒数");

	AddField("fallHeight", settings.fallHeight)
		.Category("Fall")
		.Tooltip("予告円の何メートル上から落とすか");

	AddField("fallSpeed", settings.fallSpeed)
		.Category("Fall")
		.Tooltip("落下速度 (m/s)");

	AddField("colliderRadius", settings.colliderRadius)
		.Category("Impact")
		.Tooltip("着弾時に出す判定の半径");

	AddField("impactHold", settings.impactHold)
		.Category("Impact")
		.Tooltip("判定を出しておく秒数。BreakChain が拾えるよう数フレームぶん要る");

	AddField("count", count)
		.Category("Spots")
		.Tooltip("落下地点の数。増やすと予告円が子として生える")
		.Range(0.0f, static_cast<float>(kMaxSpots));

	for (int i = 0; i < kMaxSpots; i++) {
		const std::string index = std::to_string(i);

		AddField("pos" + index, pos[i])
			.Category("Spots")
			.Tooltip("予告円を動かすと入る。ここで編集しても次のフレームで上書きされる");

		AddField("delay" + index, delay[i])
			.Category("Spots")
			.Tooltip("号令から点滅を始めるまでの秒数");
	}
}

// パス
CalyxEngine::ParamPath MeteoriteDirector::MeteoriteDirectorParam::GetParamPath() const {
	return { CalyxEngine::ParamDomain::Game, "Stage" + std::to_string(stageIndex_), "Actor/Meteorite/MeteoriteDirector" };
}
