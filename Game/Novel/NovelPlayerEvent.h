#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Novel/Asset/NovelSceneAsset.h>
#include <Engine/Novel/Runtime/NovelPlayer.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Scene/Reference/SceneObjectReference.h>

#include <memory>

CALYX_OBJECT(Category = Event, DisplayName = "Novel Player")
class NovelPlayerEvent final : public BaseEventObject {
public:
  NovelPlayerEvent();
  void Initialize() override;
  void AlwaysUpdate(float dt) override;
  void DerivativeGui() override;
  void ApplyDerivedConfigFromJson(const nlohmann::json &root,
                                  const nlohmann::json *derived) override;
  void ExtractDerivedConfigToJson(nlohmann::json &root,
                                  nlohmann::json &derived) const override;
  void RemapSceneObjectReferences(
      const std::unordered_map<Guid, Guid> &guidMap) override;
  std::string_view GetObjectClassName() const override {
    return "NovelPlayerEvent";
  }

  // 選択されたNovelSceneAssetを読み込み、Runtime Playerで再生する。
  void Play();

  // 再生を停止し、Playerが生成した表示オブジェクトを片付ける。
  void Stop();

private:
  // 入力状態を初期化し、スキップ案内を通常表示へ戻す。
  void ResetAdvanceInput();

  // Inspectorで指定された通常ボタンとスキップ案内の表示を切り替える。
  void UpdateGuideVisibility(bool showSkipGuide);

  // 再生終了を一度だけ処理し、必要なら次のシーンへ遷移する。
  void HandlePlaybackFinished();

  Guid novelSceneGuid_{};
  bool playOnStart_ = false;

  // 入力機器と長押し時間はPlayerへ持ち込まず、起動役のEventで管理する。
  bool enableSpaceInput_ = true;
  bool enableGamepadAInput_ = true;
  bool enableHoldToSkip_ = true;
  float skipGuideDelay_ = 1.5f;
  float skipExecutionDelay_ = 3.0f;

  // ボタンUIはHierarchy上の任意のSceneObjectをGUIDで参照する。
  CalyxEngine::SceneObjectRef<SceneObject> advanceGuide_{};
  CalyxEngine::SceneObjectRef<SceneObject> skipGuide_{};

  bool transitionOnFinished_ = false;
  Guid destinationSceneGuid_{};

  float inputHoldDuration_ = 0.0f;
  bool wasInputHeld_ = false;
  bool skipExecuted_ = false;
  bool finishHandled_ = false;
  CalyxEngine::NovelSceneAsset scene_;
  std::unique_ptr<CalyxEngine::NovelPlayer> player_;
};
