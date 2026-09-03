#include "Obstacle.h"
#include <Engine/Scene/Utility/SceneUtility.h>

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace CalyxEngine;

namespace {
	// パラメータはGUI上ではfloatだが、棘は1ブロックに1本配置する。
	// そのため、個数と座標の計算で共通の丸め方を使う。
	int ToBlockCount(float size) noexcept {
		// Windows.hのmaxマクロと衝突しないよう、明示的に下限を選ぶ。
		const int roundedSize = static_cast<int>(std::round(size));
		return roundedSize < 1 ? 1 : roundedSize;
	}
}

Obstacle::Obstacle() :Actor("plane.obj", "Obstacle") {

	param_.LoadParams();
	SetTexture("Textures/Obstacle/block.png");

	// plane.obj is authored on XY. Lay it on XZ for the pseudo-2D obstacle.
	worldTransform_.eulerRotation.x = std::numbers::pi_v<float> * 0.5f;
	worldTransform_.rotationSource = RotationSource::Euler;
}

/////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::Initialize() noexcept {
	if (IsTransient()) {
		return;
	}

	// Serialized scene transforms can overwrite the constructor default.
	worldTransform_.eulerRotation.x = std::numbers::pi_v<float> * 0.5f;
	worldTransform_.rotationSource = RotationSource::Euler;
	SetTexture("Textures/Obstacle/block.png");

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
	worldTransform_.scale.y = baseScale_.y * param_.size_.y;

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

	// The texture tip is local +Y. Pitching +90 degrees lays the plane on XZ
	// and maps the tip to +Z; yaw then points it towards the relevant edge.
	constexpr float halfPi = std::numbers::pi_v<float> * 0.5f;
	constexpr float pi = std::numbers::pi_v<float>;
	const auto setThornRotation = [](WorldTransform& transform, float yaw) {
		transform.eulerRotation = { halfPi, yaw, 0.0f };
		transform.rotationSource = RotationSource::Euler;
	};

	// X方向の各ブロックの中心に、奥側(+Z)と手前側(-Z)の棘を1本ずつ配置する。
	for (int x = 0; x < width; ++x) {
		const float localX =
			-halfWidth + blockSize * 0.5f + static_cast<float>(x) * blockSize;

		// 奥側(+Z)
		{
			auto& transform =
				thorns_[thornIndex++]->GetWorldTransform();

			transform.translation = {
				localX,
				0.0f,
				halfHeight + thornCenterOffset
			};

			setThornRotation(transform, 0.0f);
		}

		// 手前側(-Z)
		{
			auto& transform =
				thorns_[thornIndex++]->GetWorldTransform();

			transform.translation = {
				localX,
				0.0f,
				-halfHeight - thornCenterOffset
			};

			setThornRotation(transform, pi);
		}
	}

	// Z方向の各ブロックの中心に、左側(-X)と右側(+X)の棘を1本ずつ配置する。
	for (int z = 0; z < height; ++z) {
		const float localZ =
			-halfHeight + blockSize * 0.5f + static_cast<float>(z) * blockSize;

		// 左側(-X)
		{
			auto& transform =
				thorns_[thornIndex++]->GetWorldTransform();

			transform.translation = {
				-halfWidth - thornCenterOffset,
				0.0f,
				localZ
			};

			setThornRotation(transform, -halfPi);
		}

		// 右側(+X)
		{
			auto& transform =
				thorns_[thornIndex++]->GetWorldTransform();

			transform.translation = {
				halfWidth + thornCenterOffset,
				0.0f,
				localZ
			};

			setThornRotation(transform, halfPi);
		}
	}
}

void Obstacle::RebuildThorns(int width, int height) {
	// 上下にwidth本ずつ、左右にheight本ずつ必要。
	const size_t requiredCount = static_cast<size_t>((width + height) * 2);

	// 足りない分だけ生成
	while (thorns_.size() < requiredCount) {
		CalyxEngine::Vector3 dumy = { 0.0f,0.0f,0.0f };
		auto thorn = SceneAPI::InstantiatePrefabRoot<Thorn>("thorn.prefab", dumy);

		// 親の拡大率を継承すると棘自体まで伸びるため、inheritScaleはfalseにする。
		thorn->SetParent(shared_from_this(), false);
		// Both parent and thorn planes have their own XZ rotation. Do not apply
		// the obstacle rotation to the thorn a second time.
		thorn->GetWorldTransform().inheritRotate = false;

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
