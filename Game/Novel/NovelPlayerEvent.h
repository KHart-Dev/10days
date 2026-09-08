#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Novel/Asset/NovelSceneAsset.h>
#include <Engine/Novel/Runtime/NovelPlayer.h>
#include <Engine/Objects/Event/BaseEventObject.h>

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
  std::string_view GetObjectClassName() const override {
    return "NovelPlayerEvent";
  }
  // Starts the selected scene asset through the runtime player.
  void Play();

  // Stops playback and removes the presentation objects.
  void Stop();

private:
  Guid novelSceneGuid_{};
  bool playOnStart_ = false;
  CalyxEngine::NovelSceneAsset scene_;
  std::unique_ptr<CalyxEngine::NovelPlayer> player_;
};
