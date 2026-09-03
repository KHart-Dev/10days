#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Math/Vector3.h>

// game
#include "Thorn.h"

/*-----------------------------------------------------------------------------------------
 * Obstacle
 * - 障害物
 * - playerが衝突したらダメージを受けるような障害物
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Obstacle", Icon = "UI/Tool/cube.dds")
class Obstacle final
	:public Actor {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	Obstacle();
	~Obstacle() = default;

	/**
	 * @brief 初期化
	 */
	void Initialize()noexcept override;
	/**
	 * @brief 更新
	 * @param dtca
	 */
	void AlwaysUpdate(float dt)override;
	/**
	 * \brief 派生パラメータGUI
	 */
	void DerivativeGui() override;

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	/**
	 * @brief 現在の障害物サイズから、全ての棘のローカル座標を計算する
	*/
	void ComputeOffset()noexcept;
	/**
	 * @brief 外周に必要な数になるよう棘を増減する
	 * @param width X方向のブロック数
	 * @param height Z方向のブロック数
	 */
	void RebuildThorns(int width, int height);

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	/**
	 * @brief パラメータ
	 */
	struct ObstacleParam
		: public CalyxEngine::SerializableObject {
		ObstacleParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		CalyxEngine::Vector2 size_{ 1.0f,1.0f };		//< 5でサイズ1 サイズ１ごとに棘が増える
	}param_;

	// plane.obj is 2x2, so 0.5 corresponds to one obstacle block.
	CalyxEngine::Vector3 baseScale_{ 0.5f,0.5f,0.5f };	//< 基準になる大きさ

	std::vector<std::shared_ptr<Thorn>> thorns_;		//< 棘
	
};
