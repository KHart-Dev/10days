#include "Obstacle.h"
#include <Engine/Scene/Utility/SceneUtility.h>
Obstacle::Obstacle() :Actor("debugCube.obj", "Obstacle") {

	param_.LoadParams();
}

/////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::Initialize() noexcept {

    if (!thorns_.empty()) {
        return;
    }

    const int width =
        static_cast<int>(std::round(param_.size_.x));

    const int height =
        static_cast<int>(std::round(param_.size_.y));

    const size_t thornsCount =
        static_cast<size_t>(width * 2 + height * 2);

    thorns_.reserve(thornsCount);

    constexpr float blockSize = 1.0f;
    constexpr float thornOffset = 0.5f;

    const float halfWidth =
        static_cast<float>(width) * blockSize * 0.5f;

    const float halfHeight =
        static_cast<float>(height) * blockSize * 0.5f;

    // 上下
    for (int x = 0; x < width; ++x) {

        const float localX =
            -halfWidth +
            blockSize * 0.5f +
            static_cast<float>(x) * blockSize;

        // 上
        {
            auto thorn = SceneAPI::Instantiate<Thorn>();
            thorn->SetParent(shared_from_this(), false);

            thorn->GetWorldTransform().translation = {
                localX,
                0.0f,
                halfHeight + thornOffset
            };

            thorns_.push_back(thorn);
        }

        // 下
        {
            auto thorn = SceneAPI::Instantiate<Thorn>();
            thorn->SetParent(shared_from_this(), false);

            thorn->GetWorldTransform().translation = {
                localX,
                0.0f,
                -halfHeight - thornOffset
            };

            thorns_.push_back(thorn);
        }
    }

    // 左右
    for (int z = 0; z < height; ++z) {

        const float localZ =
            -halfHeight +
            blockSize * 0.5f +
            static_cast<float>(z) * blockSize;

        // 左
        {
            auto thorn = SceneAPI::Instantiate<Thorn>();
            thorn->SetParent(shared_from_this(), false);

            thorn->GetWorldTransform().translation = {
                -halfWidth - thornOffset,
                0.0f,
                localZ
            };

            thorns_.push_back(thorn);
        }

        // 右
        {
            auto thorn = SceneAPI::Instantiate<Thorn>();
            thorn->SetParent(shared_from_this(),false);

            thorn->GetWorldTransform().translation = {
                halfWidth + thornOffset,
                0.0f,
                localZ
            };

            thorns_.push_back(thorn);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////
void Obstacle::AlwaysUpdate(float dt) {

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
void Obstacle::ComputeOffset() noexcept {}

void Obstacle::RebuildThorns(int width, int height) {
    const size_t requiredCount =
        static_cast<size_t>(width * 2 + height * 2);

    // 足りない分だけ生成
    while (thorns_.size() < requiredCount) {

        auto thorn = SceneAPI::Instantiate<Thorn>();

        thorn->SetParent(shared_from_this(), false);

        thorns_.push_back(thorn);
    }

    // 多すぎる場合
    while (thorns_.size() > requiredCount) {

        // SceneAPI側にDestroyがあるならここで削除
        SceneAPI::Remove(thorns_.back());

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
