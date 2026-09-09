#include "NovelPlayerEvent.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

#include <Game/Audio/GameAudio.h>

#include <algorithm>

NovelPlayerEvent::NovelPlayerEvent()
    : BaseEventObject("NovelPlayerEvent"),
      player_(std::make_unique<CalyxEngine::NovelPlayer>()) {
}

void NovelPlayerEvent::Initialize() {
  BaseEventObject::Initialize();
  InitializeSkipUi();
  UpdateGuideVisibility(false);

  GameAudio::PlayBgm(GameAudio::kBgmStory);

  if (playOnStart_) {
    Play();
  }
}

void NovelPlayerEvent::AlwaysUpdate(float dt) {
  BaseEventObject::AlwaysUpdate(dt);
  if (!player_) {
    return;
  }

  player_->Update(dt);

  if (player_->IsFinished()) {
    HandlePlaybackFinished();
    return;
  }

  if (!player_->IsPlaying()) {
    return;
  }

  // テキスト入力中の Space は入力欄だけに渡し、ノベル操作には使わない。
  // 押下状態もリセットして、入力終了直後のキーリリースで進まないようにする。
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantTextInput) {
    ResetAdvanceInput();
    return;
  }

  // SpaceとゲームパッドAを同じ「決定入力」としてまとめる。
  // 短押しと長押しを区別するため、押した瞬間ではなく離した瞬間にNextする。
  const bool spaceHeld =
      enableSpaceInput_ && CalyxFoundation::Input::PushKey(DIK_SPACE);
  const bool gamepadAHeld =
      enableGamepadAInput_ &&
      CalyxFoundation::Input::PushGamepadButton(
          CalyxFoundation::PadButton::A);
  const bool inputHeld = spaceHeld || gamepadAHeld;

  if (inputHeld) {
    inputHoldDuration_ += (std::max)(dt, 0.0f);
    wasInputHeld_ = true;

    if (enableHoldToSkip_ && inputHoldDuration_ >= skipGuideDelay_) {
      UpdateGuideVisibility(true);
      UpdateSkipUi(inputHoldDuration_);
    }

    if (enableHoldToSkip_ && inputHoldDuration_ >= skipExecutionDelay_) {
      skipExecuted_ = true;
      //player_->SkipToEnd();
      HandlePlaybackFinished();
    }

    return;
  }

  // Inputにはキーボード／パッドのRelease APIがないため、
  // 前フレームの保持状態を使って離した瞬間を判定する。
  if (wasInputHeld_ && !skipExecuted_) {
    player_->Next();
  }

  ResetAdvanceInput();
}

void NovelPlayerEvent::Play() {
  const AssetRecord *record =
      novelSceneGuid_.isValid()
          ? AssetDatabase::GetInstance()->Get(novelSceneGuid_)
          : nullptr;
  if (!record || record->type != AssetType::NovelScene) {
    return;
  }

  if (!scene_.Load(record->sourcePath)) {
    return;
  }

  player_->SetScene(&scene_);
  player_->Play();
  finishHandled_ = false;
  ResetAdvanceInput();
  UpdateGuideVisibility(false);
}

void NovelPlayerEvent::Stop() {
  if (player_) {
    player_->Stop();
  }

  finishHandled_ = false;
  ResetAdvanceInput();
  UpdateGuideVisibility(false);
}

void NovelPlayerEvent::DerivativeGui() {
  BaseEventObject::DerivativeGui();
  ImGui::SeparatorText("ノベルプレイヤー");

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
  ImGui::TextWrapped("ノベルシーン: %s", filename.c_str());
  ImGui::Checkbox("開始時に再生", &playOnStart_);

  ImGui::SeparatorText("進行入力");
  ImGui::Checkbox("Spaceキーで進む", &enableSpaceInput_);
  ImGui::Checkbox("ゲームパッドAで進む", &enableGamepadAInput_);
  ImGui::Checkbox("長押しスキップを有効にする", &enableHoldToSkip_);

  if (!enableHoldToSkip_) {
    ImGui::BeginDisabled();
  }
  ImGui::DragFloat("スキップ表示開始（秒）", &skipGuideDelay_, 0.05f,
                   0.1f, 30.0f, "%.2f");
  ImGui::DragFloat("スキップ実行（秒）", &skipExecutionDelay_, 0.05f,
                   0.1f, 30.0f, "%.2f");
  skipGuideDelay_ = (std::max)(skipGuideDelay_, 0.1f);
  skipExecutionDelay_ =
      (std::max)(skipExecutionDelay_, skipGuideDelay_);

  if (!enableHoldToSkip_) {
    ImGui::EndDisabled();
  }

  // HierarchyからSceneObjectをドロップして、通常表示とスキップ表示を指定する。
  GuiCmd::SceneObjectReferenceField("通常ボタンUI", advanceGuide_);

  ImGui::SeparatorText("再生終了後");
  ImGui::Checkbox("終了時にシーン遷移", &transitionOnFinished_);

  if (!transitionOnFinished_) {
    ImGui::BeginDisabled();
  }

  Guid destinationGuid = destinationSceneGuid_;
  if (CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Scene,
                                                   &destinationGuid)) {
    destinationSceneGuid_ = destinationGuid;
  }

  const AssetRecord *destinationRecord =
      destinationSceneGuid_.isValid()
          ? AssetDatabase::GetInstance()->Get(destinationSceneGuid_)
          : nullptr;
  const std::string destinationName =
      destinationRecord && destinationRecord->type == AssetType::Scene
          ? destinationRecord->sourcePath.filename().string()
          : "（未設定）";
  ImGui::TextWrapped("遷移先シーン: %s", destinationName.c_str());

  if (!transitionOnFinished_) {
    ImGui::EndDisabled();
  }

  if (ImGui::Button("再生")) {
    Play();
  }

  ImGui::SameLine();

  if (ImGui::Button("停止")) {
    Stop();
  }
}

void NovelPlayerEvent::ApplyDerivedConfigFromJson(
    [[maybe_unused]] const nlohmann::json &root,
    const nlohmann::json *derived) {
  if (!derived) {
    return;
  }

  novelSceneGuid_ = derived->value("novelSceneGuid", Guid{});
  playOnStart_ = derived->value("playOnStart", false);
  enableSpaceInput_ = derived->value("enableSpaceInput", true);
  enableGamepadAInput_ = derived->value("enableGamepadAInput", true);
  enableHoldToSkip_ = derived->value("enableHoldToSkip", true);
  skipGuideDelay_ = derived->value("skipGuideDelay", 1.5f);
  skipExecutionDelay_ = derived->value("skipExecutionDelay", 3.0f);
  advanceGuide_.SetGuid(derived->value("advanceGuideGuid", Guid{}));
  transitionOnFinished_ = derived->value("transitionOnFinished", false);
  destinationSceneGuid_ = derived->value("destinationSceneGuid", Guid{});
}

void NovelPlayerEvent::ExtractDerivedConfigToJson(
    [[maybe_unused]] nlohmann::json &root, nlohmann::json &derived) const {
  derived["novelSceneGuid"] = novelSceneGuid_;
  derived["playOnStart"] = playOnStart_;
  derived["enableSpaceInput"] = enableSpaceInput_;
  derived["enableGamepadAInput"] = enableGamepadAInput_;
  derived["enableHoldToSkip"] = enableHoldToSkip_;
  derived["skipGuideDelay"] = skipGuideDelay_;
  derived["skipExecutionDelay"] = skipExecutionDelay_;
  derived["advanceGuideGuid"] = advanceGuide_.GetGuid();
  derived["transitionOnFinished"] = transitionOnFinished_;
  derived["destinationSceneGuid"] = destinationSceneGuid_;
}

void NovelPlayerEvent::RemapSceneObjectReferences(
    const std::unordered_map<Guid, Guid> &guidMap) {
  BaseEventObject::RemapSceneObjectReferences(guidMap);
  advanceGuide_.Remap(guidMap);
}

void NovelPlayerEvent::ResetAdvanceInput() {
  inputHoldDuration_ = 0.0f;
  wasInputHeld_ = false;
  skipExecuted_ = false;
  UpdateGuideVisibility(false);
}

void NovelPlayerEvent::InitializeSkipUi() {
    skipUi_ = SceneAPI::Instantiate<UiSprite>("Textures/Story/UI/skipText.png","skipTextUI");
    if (!skipUi_) {
        return;
    } else {
        skipUi_->GetWorldTransform().scale = { 150.0f,90.0f,1.0f };
		skipUi_->GetWorldTransform().translation = { 988.0f, 610.0f, 0.0f };
        skipUi_->SetOrderInLayer(111);
        skipUi_->GetWorldTransform().Update();
    }
	skipFrameUi_ = SceneAPI::Instantiate<UiSprite>("Textures/Story/UI/skipBackground.png","skipFrameUI");
    if (!skipFrameUi_) {
        return;
    } else {
		skipFrameUi_->GetWorldTransform().scale = { 150.0f,90.0f,1.0f };
		skipFrameUi_->GetWorldTransform().translation = { 988.0f, 610.0f, 0.0f };  
		skipFrameUi_->SetOrderInLayer(109);
        skipFrameUi_->GetWorldTransform().Update();
    }
	skipBackUi_ = SceneAPI::Instantiate<UiSprite>("Textures/Story/UI/skipGauge.png","skipBackUI");
	if (!skipBackUi_) {
		return;
    } else {
		skipBackUi_->GetWorldTransform().scale = { 150.0f,90.0f,1.0f };
		skipBackUi_->GetWorldTransform().translation = { 988.0f, 610.0f, 0.0f };
        skipBackUi_->SetOrderInLayer(110);
        skipBackUi_->GetWorldTransform().Update();
    }
}

void NovelPlayerEvent::UpdateGuideVisibility(bool showSkipGuide) {
    if (auto advanceObject = advanceGuide_.Resolve()) {
        advanceObject->SetDrawEnable(!showSkipGuide);
    }
    if (skipUi_) {
        skipUi_->SetDrawEnable(showSkipGuide);
    }
    if (skipFrameUi_) {
        skipFrameUi_->SetDrawEnable(showSkipGuide);
    }
    if (skipBackUi_) {
        skipBackUi_->SetDrawEnable(showSkipGuide);
    }
}

void NovelPlayerEvent::UpdateSkipUi(float dt) {
    
    float scale = (dt - skipGuideDelay_) / (skipExecutionDelay_ - skipGuideDelay_);
    scale = std::clamp(scale, 0.0f, 1.0f);
	skipBackUi_->GetWorldTransform().scale.x = 150.0f * scale;
    skipBackUi_->GetWorldTransform().scale.y = 90.0f;
    skipBackUi_->GetWorldTransform().translation.x = 988.0f + (1.0f - scale) * 6.0f;
    skipBackUi_->GetWorldTransform().Update();
}

void NovelPlayerEvent::HandlePlaybackFinished() {
  if (finishHandled_) {
    return;
  }

  finishHandled_ = true;
  ResetAdvanceInput();

  if (transitionOnFinished_ && destinationSceneGuid_.isValid()) {
    SceneAPI::RequestSceneChange(destinationSceneGuid_);
  }
}
