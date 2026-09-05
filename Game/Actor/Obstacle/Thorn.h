#pragma once

// engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Actor.h>

/*-----------------------------------------------------------------------------------------
 * Thorn
 * - 棘
 * - 障害物の周りについている棘
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Thorn", Icon = "UI/Tool/cube.dds"
			,Placeable = false,PrefabEditable = true,PrefabRoot = true)
class Thorn final
	:public Actor {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	Thorn();
	~Thorn()override;

	void Initialize() override;
	void Update(float dt) override;

	void OnCollisionEnter([[maybe_unused]] Collider* other) override;

	std::string_view GetObjectClassName() const override { return "Thorn"; }

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/

	void UpdateUvAnimation(float dt);
	void ApplyAnimationFrame();

private:
	// needle.png は横一列に 9 コマ
	static constexpr int kAnimationFrameCount = 9;

	int currentFrame_ = 0;
	float animationTimer_ = 0.0f;

	// 1コマの表示時間（秒）
	float frameDuration_ = 0.10f;

	CalyxEngine::EffectAsset bloodEffect_;
	CalyxEngine::EffectHandle bloodHandle_{};
};

