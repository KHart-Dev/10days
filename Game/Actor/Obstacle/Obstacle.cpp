#include "Obstacle.h"
#include <Engine/Scene/Utility/SceneUtility.h>

#include <algorithm>
#include <cmath>

namespace {
	// パラメータはGUI上ではfloatだが、棘は1ブロックに1本配置する。
	// そのため、個数と座標の計算で共通の丸め方を使う。
	int ToBlockCount(float size) noexcept {
		// Windows.hのmaxマクロと衝突しないよう、明示的に下限を選ぶ。
		const int roundedSize = static_cast<int>(std::round(size));
		return roundedSize < 1 ? 1 : roundedSize;
	}
}

Obstacle::Obstacle() :Actor("debugCube.obj", "Obstacle") {

	param_.LoadParams();
}

/////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::Initialize() noexcept {
	// Placement previews are transient. Creating scene-owned Thorn children here
	// leaves them behind when the preview is removed and duplicates them on drop.
	if (IsTransient()) {
		return;
	}

	// 生成と配置を分けることで、初期化時とサイズ変更時に同じ計算を使う。
	const int width = ToBlockCount(param_.size_.x);
	const int height = ToBlockCount(param_.size_.y);
	RebuildThorns(width, height);
	ComputeOffset();
}

/////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::AlwaysUpdate(float dt) {
	const int width = ToBlockCount(param_.size_.x);
	const int height = ToBlockCount(param_.size_.y);
	const bool shouldBuildThorns = !IsTransient();

	// GUIやパラメータからサイズが変わっても即座に外周を作り直す。
	// 個数が同じ場合、RebuildThornsのwhileループは一度も実行されない。
	if (shouldBuildThorns) {
		RebuildThorns(width, height);
		ComputeOffset();
	}

	// サイズを適用
	worldTransform_.scale.x = baseScale_.x * param_.size_.x;
	worldTransform_.scale.z = baseScale_.y * param_.size_.y;

	// 行列更新など
	Actor::AlwaysUpdate(dt);
}

/////////////////////////////////////////////////////////////////////////////////
//		デバッグgui
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::DerivativeGui() {
	Actor::DerivativeGui();

	ImGui::SeparatorText("gameParam");

	ImGui::Text("トゲの数: %d", thorns_.size());
	param_.ShowGui();

	param_.size_.x = std::clamp(param_.size_.x, 1.0f, 5.0f);
	param_.size_.y = std::clamp(param_.size_.y, 1.0f, 5.0f);
}

/////////////////////////////////////////////////////////////////////////////////
//		オフセット計算
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::ComputeOffset() noexcept {
	constexpr float blockSize = 1.0f;

	// Thornは初期スケールが0.5のため、中心を障害物の面から0.5外側へ出す。
	// これにより棘の内側の端が障害物の外周に接する。
	constexpr float thornCenterOffset = 0.5f;

	const int width = ToBlockCount(param_.size_.x);
	const int height = ToBlockCount(param_.size_.y);
	const size_t requiredCount = static_cast<size_t>((width + height) * 2);

	// 単独で呼ばれた場合でも配列外アクセスしないよう防御する。
	if (thorns_.size() != requiredCount) {
		return;
	}

	const float halfWidth = static_cast<float>(width) * blockSize * 0.5f;
	const float halfHeight = static_cast<float>(height) * blockSize * 0.5f;
	size_t thornIndex = 0;

	// X方向の各ブロックの中心に、奥側(+Z)と手前側(-Z)の棘を1本ずつ配置する。
	for (int x = 0; x < width; ++x) {
		// 左端(-halfWidth)から半ブロック進めると、最初のブロック中心になる。
		const float localX =
			-halfWidth + blockSize * 0.5f + static_cast<float>(x) * blockSize;

		thorns_[thornIndex++]->GetWorldTransform().translation = {
			localX, 0.0f, halfHeight + thornCenterOffset
		};
		thorns_[thornIndex++]->GetWorldTransform().translation = {
			localX, 0.0f, -halfHeight - thornCenterOffset
		};
	}

	// Z方向も同様に、左側(-X)と右側(+X)の棘を1本ずつ配置する。
	// 角の棘は上のループと座標が重ならないため、外周は 2 * (width + height) 本になる。
	for (int z = 0; z < height; ++z) {
		const float localZ =
			-halfHeight + blockSize * 0.5f + static_cast<float>(z) * blockSize;

		thorns_[thornIndex++]->GetWorldTransform().translation = {
			-halfWidth - thornCenterOffset, 0.0f, localZ
		};
		thorns_[thornIndex++]->GetWorldTransform().translation = {
			halfWidth + thornCenterOffset, 0.0f, localZ
		};
	}
}

void Obstacle::RebuildThorns(int width, int height) {
	// 上下にwidth本ずつ、左右にheight本ずつ必要。
	const size_t requiredCount = static_cast<size_t>((width + height) * 2);

	// 足りない分だけ生成
	while (thorns_.size() < requiredCount) {

		auto thorn = SceneAPI::Instantiate<Thorn>();

		// 親の拡大率を継承すると棘自体まで伸びるため、inheritScaleはfalseにする。
		thorn->SetParent(shared_from_this(), false);

		thorns_.push_back(thorn);
	}

	// 多すぎる場合
	while (thorns_.size() > requiredCount) {

		// Destroyでシーン側へ破棄を通知してから、所有リストから取り除く。
		thorns_.back()->Destroy();

		thorns_.pop_back();
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		登録
/////////////////////////////////////////////////////////////////////////////////
Obstacle::ObstacleParam::ObstacleParam() {
	AddField("size", size_).Tooltip("大きさ").Speed(1.0f).Range(1, 5);
}

/////////////////////////////////////////////////////////////////////////////////
//		パス
/////////////////////////////////////////////////////////////////////////////////
CalyxEngine::ParamPath Obstacle::ObstacleParam::GetParamPath() const {
	return { CalyxEngine::ParamDomain::Game, "Obstacle","Actor" };
}
