#include "NovelPlayerEvent.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Foundation/Input/Input.h>
#include <externals/imgui/imgui.h>

NovelPlayerEvent::NovelPlayerEvent()
    : BaseEventObject("NovelPlayerEvent"),
      player_(std::make_unique<CalyxEngine::NovelPlayer>()) {
}

void NovelPlayerEvent::Initialize() {
  BaseEventObject::Initialize();
  if (playOnStart_)
    Play();
}

void NovelPlayerEvent::AlwaysUpdate(float dt) {
  BaseEventObject::AlwaysUpdate(dt);
  if (!player_)
    return;

  player_->Update(dt);

  // Input mapping belongs here. NovelPlayer itself stays device independent.
  if (CalyxFoundation::Input::TriggerKey(DIK_SPACE)) {
    player_->Next();
  }
}

void NovelPlayerEvent::Play() {
  const AssetRecord *record =
      novelSceneGuid_.isValid()
          ? AssetDatabase::GetInstance()->Get(novelSceneGuid_)
          : nullptr;
  if (!record || record->type != AssetType::NovelScene)
    return;
  if (!scene_.Load(record->sourcePath))
    return;

  player_->SetScene(&scene_);
  player_->Play();
}

void NovelPlayerEvent::Stop() {
  if (player_)
    player_->Stop();
}

void NovelPlayerEvent::DerivativeGui() {
  BaseEventObject::DerivativeGui();
  ImGui::SeparatorText("Novel Player");

  Guid droppedGuid = novelSceneGuid_;
  if (CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::NovelScene,
                                                   &droppedGuid)) {
    novelSceneGuid_ = droppedGuid;
  }

  const AssetRecord *record =
      novelSceneGuid_.isValid()
          ? AssetDatabase::GetInstance()->Get(novelSceneGuid_)
          : nullptr;
  const std::string filename =
      record ? record->sourcePath.filename().string() : "(none)";
  ImGui::TextWrapped("Novel Scene: %s", filename.c_str());
  ImGui::Checkbox("Play On Start", &playOnStart_);

  if (ImGui::Button("Play"))
    Play();
  ImGui::SameLine();
  if (ImGui::Button("Stop"))
    Stop();
}

void NovelPlayerEvent::ApplyDerivedConfigFromJson(
    [[maybe_unused]] const nlohmann::json &root,
    const nlohmann::json *derived) {
  if (!derived)
    return;
  novelSceneGuid_ = derived->value("novelSceneGuid", Guid{});
  playOnStart_ = derived->value("playOnStart", false);
}

void NovelPlayerEvent::ExtractDerivedConfigToJson(
    [[maybe_unused]] nlohmann::json &root, nlohmann::json &derived) const {
  derived["novelSceneGuid"] = novelSceneGuid_;
  derived["playOnStart"] = playOnStart_;
}
