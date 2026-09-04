#pragma once

namespace SceneFlow {

	// 各シーン固有の処理で遷移先を重複して定義しないよう、シーンパスを一元管理する。
	inline constexpr const char* kTitleScenePath = "Scenes/TitleScene.scene";
	inline constexpr const char* kAnimationScenePath = "Scenes/AnimationScene.scene";
	inline constexpr const char* kGameScenePath = "Scenes/DemoScene.scene";
	inline constexpr const char* kResultScenePath = "Scenes/ResultScene.scene";

	// シーン遷移と、遷移先へ引き継ぐステージ情報のみを管理する。
	// 入力、メニュー、オプション、アニメーションの終了判定は呼び出し側で実装する。
	void GoToTitle();
	void GoToAnimation();
	void StartStage(int stageIndex);
	void GoToResult();

} // namespace SceneFlow
