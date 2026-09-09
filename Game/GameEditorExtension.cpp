#include <CalyxEngine/EditorExtension.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Novel/Asset/DialogueAsset.h>
#include <Engine/Novel/Asset/NovelSceneAsset.h>
#include <Engine/Novel/Runtime/NovelPlayer.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/Object2d/SpriteSceneObject2d.h>
#include <Engine/Objects/2D/Object2d/TextSceneObject2d.h>
#include <Engine/Scene/Reference/SceneObjectReference.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numbers>

namespace {
class GameCameraEditor final : public CalyxEditor::IEditorTool {
public:
  explicit GameCameraEditor(const CalyxEditor::EditorToolContext &context)
      : context_(context) {
  }

  void OnOpen() override {
    open_ = true;
    SyncFromCamera();
  }

  void Draw() override {
    if (!open_)
      return;
    ImGui::Begin("Game Camera Editor###Game.CameraEditor", &open_);

    auto *camera = context_.GetMainCamera();
    if (!camera) {
      ImGui::TextDisabled("メインカメラが設定されていません。");
      ImGui::End();
      return;
    }

    if (context_.IsPlaying())
      ImGui::TextDisabled("実行中のカメラを編集しています");
    bool changed = ImGui::DragFloat3("Position", position_, 0.1f);
    changed |= ImGui::DragFloat3("Rotation", rotation_, 0.01f);
    changed |=
        ImGui::DragFloat("Field of View", &fieldOfView_, 0.1f, 1.0f, 179.0f);
    if (changed) {
      camera->SetCamera({position_[0], position_[1], position_[2]},
                        {rotation_[0], rotation_[1], rotation_[2]});
      camera->SetFovY(fieldOfView_ * std::numbers::pi_v<float> / 180.0f);
      camera->UpdateMatrix();
    }

    if (!context_.IsPlaying() && ImGui::Button("シーンを保存")) {
      context_.RequestSaveScene();
    }
    ImGui::End();
  }

  bool IsOpen() const override {
    return open_;
  }

private:
  void SyncFromCamera() {
    if (auto *camera = context_.GetMainCamera()) {
      const auto &position = camera->GetTranslate();
      const auto &rotation = camera->GetRotate();
      position_[0] = position.x;
      position_[1] = position.y;
      position_[2] = position.z;
      rotation_[0] = rotation.x;
      rotation_[1] = rotation.y;
      rotation_[2] = rotation.z;
      fieldOfView_ = camera->GetFovY() * 180.0f / std::numbers::pi_v<float>;
    }
  }

  CalyxEditor::EditorToolContext context_;
  bool open_ = true;
  float position_[3]{};
  float rotation_[3]{};
  float fieldOfView_ = 60.0f;
};

// Asset is pure data. These buffers only bridge std::string with ImGui editing.
class DialogueEditor final : public CalyxEditor::IEditorTool {
public:
  explicit DialogueEditor(const CalyxEditor::EditorToolContext &) {
  }
  void OnOpen() override {
    open_ = true;
  }

  void Draw() override {
    if (!open_)
      return;
    ImGui::Begin("Dialogue Editor###Novel.DialogueEditor", &open_);
    DrawToolbar();
    DrawAssetSelector();

    const auto flags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("DialogueColumns", 2, flags)) {
      ImGui::TableSetupColumn("セリフ一覧", ImGuiTableColumnFlags_WidthFixed,
                              260.0f);
      ImGui::TableSetupColumn("セリフ設定");
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      DrawLineList();
      ImGui::TableSetColumnIndex(1);
      DrawInspector();
      ImGui::EndTable();
    }
    ImGui::End();
  }

  bool IsOpen() const override {
    return open_;
  }

private:
  void DrawToolbar() {
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("アセット名", assetName_.data(), assetName_.size());
    ImGui::SameLine();
    if (ImGui::Button("新規作成")) {
      asset_ = {};
      selectedGuid_ = {};
      currentPath_.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("保存"))
      Save();
  }

  void DrawAssetSelector() {
    const std::string current =
        currentPath_.empty() ? "(未選択)" : currentPath_.filename().string();
    if (!ImGui::BeginCombo("会話アセットを読み込む", current.c_str()))
      return;

    for (const AssetRecord *record : AssetDatabase::GetInstance()->GetView()) {
      if (!record || record->type != AssetType::Dialogue)
        continue;
      const std::string filename = record->sourcePath.filename().string();
      if (ImGui::Selectable(filename.c_str()) &&
          asset_.Load(record->sourcePath)) {
        currentPath_ = record->sourcePath;
        CopyText(assetName_, record->sourcePath.stem().string());
        selectedGuid_ = {};
      }
    }
    ImGui::EndCombo();
  }

  void DrawLineList() {
    int index = 1;
    for (auto &line : asset_.GetLines()) {
      char label[256]{};
      std::snprintf(label, sizeof(label), "%03d %s", index++,
                    line.speaker_.c_str());
      if (ImGui::Selectable(label, line.guid_ == selectedGuid_)) {
        selectedGuid_ = line.guid_;
        SyncBuffers(line);
      }
    }

    if (ImGui::Button("セリフを追加")) {
      auto &line = asset_.AddLine();
      selectedGuid_ = line.guid_;
      SyncBuffers(line);
    }
    ImGui::SameLine();
    if (ImGui::Button("セリフを削除") && selectedGuid_.isValid()) {
      asset_.RemoveLine(selectedGuid_);
      selectedGuid_ = {};
    }
    if (ImGui::Button("上へ"))
      asset_.MoveLine(selectedGuid_, -1);
    ImGui::SameLine();
    if (ImGui::Button("下へ"))
      asset_.MoveLine(selectedGuid_, 1);
  }

  void DrawInspector() {
    auto *line = asset_.FindLine(selectedGuid_);
    if (!line)
      return;

    if (ImGui::InputText("話者名", speakerBuffer_.data(),
                         speakerBuffer_.size())) {
      line->speaker_ = speakerBuffer_.data();
    }
    if (ImGui::InputTextMultiline("セリフ本文", textBuffer_.data(),
                                  textBuffer_.size(), ImVec2(-1.0f, 180.0f))) {
      line->text_ = textBuffer_.data();
    }
    ImGui::DragFloat("1秒あたりの表示文字数", &line->charactersPerSecond_, 0.5f,
                     0.01f, 1000.0f);
    ImGui::Checkbox("全文表示後に自動で進む", &line->autoAdvance_);
    if (line->autoAdvance_) {
      ImGui::DragFloat("自動で進むまでの待ち時間", &line->autoAdvanceDelay_,
                       0.05f, 0.0f, 60.0f);
    }
  }

  template <size_t Size>
  static void CopyText(std::array<char, Size> &destination,
                       const std::string &source) {
    destination.fill('\0');
    const size_t count = (std::min)(source.size(), destination.size() - 1);
    std::memcpy(destination.data(), source.data(), count);
  }

  void SyncBuffers(const CalyxEngine::DialogueLine &line) {
    CopyText(speakerBuffer_, line.speaker_);
    CopyText(textBuffer_, line.text_);
  }

  void Save() {
    const std::string name = assetName_.data();
    if (name.empty())
      return;

    auto *database = AssetDatabase::GetInstance();
    const auto path =
        database->GetRoot() / "Novel" / "Dialogues" / (name + ".dialogue");
    if (asset_.Save(path)) {
      currentPath_ = path;
      database->RegisterOrUpdate(path, AssetType::Dialogue);
    }
  }

  bool open_ = true;
  CalyxEngine::DialogueAsset asset_;
  Guid selectedGuid_{};
  std::filesystem::path currentPath_;
  std::array<char, 128> assetName_{};
  std::array<char, 256> speakerBuffer_{};
  std::array<char, 8192> textBuffer_{};
};

const char *EventTypeName(CalyxEngine::NovelEventType type) {
  using Type = CalyxEngine::NovelEventType;
  switch (type) {
  case Type::Dialogue:
    return "会話";
  case Type::ShowImage:
    return "画像を表示";
  case Type::HideImage:
    return "画像を非表示";
  case Type::ChangeBackground:
    return "背景を変更";
  case Type::Wait:
    return "待機";
  case Type::SceneObject:
    return "シーンオブジェクト";
  }
  return "会話";
}

class NovelSceneEditor final : public CalyxEditor::IEditorTool {
public:
  explicit NovelSceneEditor(const CalyxEditor::EditorToolContext &context)
      : context_(context) {
  }

  void OnOpen() override {
    open_ = true;
    lastUpdate_ = std::chrono::steady_clock::now();
  }

  void OnClose() override {
    player_.Stop();
  }

  void Update() override {
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - lastUpdate_).count();
    lastUpdate_ = now;
    if (player_.IsPlaying()) {
      player_.Update((std::min)(dt, 0.1f));
    }
  }

  void Draw() override {
    if (!open_) {
      return;
    }
    ImGui::Begin("Novel Scene Editor###Novel.SceneEditor", &open_);
    DrawToolbar();
    DrawAssetSelector();

    DrawWorkflowGuide();

    const auto flags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("NovelColumns", 2, flags)) {
      ImGui::TableSetupColumn("Sequence", ImGuiTableColumnFlags_WidthStretch,
                              0.62f);
      ImGui::TableSetupColumn("Clip設定", ImGuiTableColumnFlags_WidthStretch,
                              0.38f);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      DrawEventList();
      ImGui::TableSetColumnIndex(1);
      DrawPreview();
      DrawInspector();
      DrawTextSettings();
      ImGui::EndTable();
    }
    ImGui::End();
  }

  bool IsOpen() const override {
    return open_;
  }

private:
  void DrawWorkflowGuide() {
    ImGui::TextDisabled(
        "1. シーンを作成または読込  2. Timelineへ追加  "
        "3. Clipを選択して設定  4. 保存  5. Viewportで再生確認");
  }

  void DrawToolbar() {
    ImGui::InputText("アセット名", assetName_.data(), assetName_.size());
    ImGui::SameLine();
    if (ImGui::Button("新規作成")) {
      player_.Stop();
      asset_ = {};
      selectedGuid_ = {};
      currentPath_.clear();
      saveMessage_ = "未保存の新規シーン";
    }
    ImGui::SameLine();
    if (ImGui::Button("シーンを保存")) {
      Save();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", saveMessage_.c_str());
  }

  void DrawAssetSelector() {
    const std::string current =
        currentPath_.empty() ? "(未選択)" : currentPath_.filename().string();
    if (!ImGui::BeginCombo("ノベルシーンを読み込む", current.c_str())) {
      return;
    }

    for (const AssetRecord *record : AssetDatabase::GetInstance()->GetView()) {
      if (!record || record->type != AssetType::NovelScene) {
        continue;
      }
      const std::string filename = record->sourcePath.filename().string();
      if (ImGui::Selectable(filename.c_str())) {
        player_.Stop();
        if (asset_.Load(record->sourcePath)) {
          currentPath_ = record->sourcePath;
          CopyName(record->sourcePath.stem().string());
          selectedGuid_ = {};
          saveMessage_ = "読み込み完了: " + filename;
        }
      }
    }
    ImGui::EndCombo();
  }

  void DrawEventList() {
    ImGui::SeparatorText("タイムライン");
    ImGui::TextDisabled("HierarchyのTextまたはSpriteを「＋」へドロップします。"
                        "Clipをドラッグすると順番を変更できます。");
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("表示倍率", &timelineZoom_, 60.0f, 240.0f, "%.0f px/秒");

    auto &events = asset_.GetEvents();
    ImGui::BeginChild("TimelineCanvas", ImVec2(0.0f, 190.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (size_t index = 0; index <= events.size(); ++index) {
      DrawInsertButton(index);
      if (index < events.size()) {
        ImGui::SameLine();
        DrawEventCard(events[index], index);
        ImGui::SameLine();
      }
    }
    ImGui::EndChild();

    // Dropping onto empty timeline space appends a clip. The + markers remain
    // available when an exact insertion position is required.
    if (ImGui::BeginDragDropTarget()) {
      const ImGuiPayload *payload =
          ImGui::AcceptDragDropPayload("SceneObjectPtr");
      if (payload && payload->Data &&
          payload->DataSize == sizeof(SceneObject *)) {
        SceneObject *object = *static_cast<SceneObject **>(payload->Data);
        InsertSceneObject(events.size(), object);
      }
      ImGui::EndDragDropTarget();
    }

    if (requestInsertPopup_) {
      ImGui::OpenPopup("Insert Event");
      requestInsertPopup_ = false;
    }
    DrawInsertPopup();
    ApplyPendingMove();

    if (selectedGuid_.isValid()) {
      ImGui::Separator();
      if (ImGui::Button("選択中のClipを削除")) {
        asset_.RemoveEvent(selectedGuid_);
        selectedGuid_ = {};
        saveMessage_ = "未保存の変更があります";
      }
    }
  }

  // The plus marker represents an exact insertion point in the sequence.
  void DrawInsertButton(size_t index) {
    ImGui::PushID(static_cast<int>(index));
    if (ImGui::Button("+", ImVec2(28.0f, 112.0f))) {
      insertionIndex_ = index;
      requestInsertPopup_ = true;
    }

    // Hierarchy payloads carry a non-owning pointer. Only the object's GUID
    // is copied into the Novel asset.
    if (ImGui::BeginDragDropTarget()) {
      const ImGuiPayload *payload =
          ImGui::AcceptDragDropPayload("SceneObjectPtr");
      if (payload && payload->Data &&
          payload->DataSize == sizeof(SceneObject *)) {
        SceneObject *object = *static_cast<SceneObject **>(payload->Data);
        InsertSceneObject(index, object);
      }
      ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
  }

  // A card exposes enough information to understand the sequence without
  // repeatedly opening the Inspector.
  void DrawEventCard(CalyxEngine::NovelEvent &event, size_t index) {
    ImGui::PushID(static_cast<int>(index));
    const bool selected = event.guid_ == selectedGuid_;
    const bool configured = IsEventConfigured(event);
    const ImVec4 color = configured ? ImVec4(0.18f, 0.26f, 0.38f, 1.0f)
                                    : ImVec4(0.48f, 0.16f, 0.14f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Header, color);
    ImGui::PushStyleColor(
        ImGuiCol_HeaderHovered,
        ImVec4(color.x + 0.08f, color.y + 0.08f, color.z + 0.08f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_HeaderActive,
        ImVec4(color.x + 0.12f, color.y + 0.12f, color.z + 0.12f, 1.0f));

    const std::string label = EventCardLabel(event, index);
    const float clipWidth = GetClipWidth(event);
    if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_None,
                          ImVec2(clipWidth, 112.0f))) {
      selectedGuid_ = event.guid_;
      SelectSceneObjectForClip(event);
    }

    if (ImGui::BeginDragDropSource()) {
      const int sourceIndex = static_cast<int>(index);
      ImGui::SetDragDropPayload("NOVEL_EVENT_INDEX", &sourceIndex,
                                sizeof(sourceIndex));
      ImGui::TextUnformatted(label.c_str());
      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
      const ImGuiPayload *payload =
          ImGui::AcceptDragDropPayload("NOVEL_EVENT_INDEX");
      if (payload && payload->DataSize == sizeof(int)) {
        pendingMoveSource_ = *static_cast<const int *>(payload->Data);
        pendingMoveTarget_ = static_cast<int>(index);
      }
      ImGui::EndDragDropTarget();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopID();
  }

  // SceneObject clips use the shared editor selection pipeline so the normal
  // Inspector and Guizmo edit the same object selected on the Timeline.
  void SelectSceneObjectForClip(const CalyxEngine::NovelEvent &event) {
    if (event.type_ != CalyxEngine::NovelEventType::SceneObject) {
      return;
    }

    const auto &clip =
        std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_);
    if (clip.sceneObjectGuid_.isValid()) {
      context_.RequestSelectSceneObject(clip.sceneObjectGuid_);
    }
  }

  float GetClipWidth(const CalyxEngine::NovelEvent &event) const {
    float duration = 0.0f;
    if (event.type_ == CalyxEngine::NovelEventType::Wait) {
      duration = std::get<CalyxEngine::NovelWaitEvent>(event.data_).duration_;
    }
    if (event.type_ == CalyxEngine::NovelEventType::SceneObject) {
      const auto &clip =
          std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_);
      if (clip.advanceMode_ == CalyxEngine::NovelClipAdvanceMode::Time) {
        duration = clip.duration_;
      }
    }

    if (duration <= 0.0f) {
      return 220.0f;
    }
    return (std::clamp)(duration * timelineZoom_, 110.0f, 640.0f);
  }

  std::string EventCardLabel(const CalyxEngine::NovelEvent &event,
                             size_t index) {
    const std::string summary =
        EventSummary(event, static_cast<int>(index + 1));
    const std::string status =
        IsEventConfigured(event) ? "設定済み" : "設定が必要";
    return summary + "\n" + ClipAdvanceLabel(event) + "\n" + status;
  }

  std::string ClipAdvanceLabel(const CalyxEngine::NovelEvent &event) const {
    if (event.type_ == CalyxEngine::NovelEventType::Wait) {
      const float duration =
          std::get<CalyxEngine::NovelWaitEvent>(event.data_).duration_;
      return "時間  " + std::to_string(duration) + " 秒";
    }
    if (event.type_ == CalyxEngine::NovelEventType::SceneObject) {
      const auto &clip =
          std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_);
      if (clip.advanceMode_ == CalyxEngine::NovelClipAdvanceMode::Time) {
        return "時間  " + std::to_string(clip.duration_) + " 秒";
      }
      return "ボタン入力待ち";
    }
    if (event.type_ == CalyxEngine::NovelEventType::Dialogue) {
      return "ボタン入力待ち";
    }
    return "即時実行";
  }

  bool IsEventConfigured(const CalyxEngine::NovelEvent &event) const {
    switch (event.type_) {
    case CalyxEngine::NovelEventType::Dialogue: {
      const auto &data = std::get<CalyxEngine::NovelDialogueEvent>(event.data_);
      return data.dialogueAssetGuid_.isValid() &&
             data.dialogueLineGuid_.isValid();
    }
    case CalyxEngine::NovelEventType::ShowImage:
      return std::get<CalyxEngine::NovelShowImageEvent>(event.data_)
          .textureGuid_.isValid();
    case CalyxEngine::NovelEventType::HideImage:
      return std::get<CalyxEngine::NovelHideImageEvent>(event.data_)
          .targetGuid_.isValid();
    case CalyxEngine::NovelEventType::ChangeBackground:
      return std::get<CalyxEngine::NovelBackgroundEvent>(event.data_)
          .textureGuid_.isValid();
    case CalyxEngine::NovelEventType::Wait:
      return true;
    case CalyxEngine::NovelEventType::SceneObject:
      return std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_)
          .sceneObjectGuid_.isValid();
    }
    return false;
  }

  void DrawInsertPopup() {
    if (!ImGui::BeginPopup("Insert Event")) {
      return;
    }

    ImGui::TextDisabled("この位置へイベントを挿入します");
    DrawDialogueInsertMenu();
    DrawInsertMenuItem("画像を表示", CalyxEngine::NovelEventType::ShowImage);
    DrawInsertMenuItem("画像を非表示", CalyxEngine::NovelEventType::HideImage);
    DrawInsertMenuItem("背景を変更",
                       CalyxEngine::NovelEventType::ChangeBackground);
    DrawInsertMenuItem("待機", CalyxEngine::NovelEventType::Wait);
    ImGui::EndPopup();
  }

  void DrawInsertMenuItem(const char *label, CalyxEngine::NovelEventType type) {
    if (ImGui::MenuItem(label)) {
      InsertEvent(insertionIndex_, type);
    }
  }

  // Dialogue lines are inserted fully configured. Authors choose the asset and
  // line in the insertion menu instead of creating an empty event first.
  void DrawDialogueInsertMenu() {
    if (!ImGui::BeginMenu("会話")) {
      return;
    }

    bool foundDialogueAsset = false;
    for (const AssetRecord *record : AssetDatabase::GetInstance()->GetView()) {
      if (!record || record->type != AssetType::Dialogue) {
        continue;
      }

      foundDialogueAsset = true;
      const std::string assetName = record->sourcePath.filename().string();
      if (!ImGui::BeginMenu(assetName.c_str())) {
        continue;
      }

      CalyxEngine::DialogueAsset dialogue;
      if (!dialogue.Load(record->sourcePath)) {
        ImGui::TextDisabled("会話アセットの読み込みに失敗しました");
        ImGui::EndMenu();
        continue;
      }

      int lineIndex = 1;
      for (const auto &line : dialogue.GetLines()) {
        const std::string label = std::to_string(lineIndex++) + "  " +
                                  line.speaker_ + " | " + line.text_;
        if (ImGui::MenuItem(label.c_str())) {
          InsertDialogue(insertionIndex_, record->guid, line.guid_);
        }
      }
      ImGui::EndMenu();
    }

    if (!foundDialogueAsset) {
      ImGui::TextDisabled(
          "先にDialogue Editorで会話を作成して保存してください");
    }
    ImGui::EndMenu();
  }

  void InsertDialogue(size_t index, const Guid &dialogueAssetGuid,
                      const Guid &dialogueLineGuid) {
    auto &events = asset_.GetEvents();
    const size_t safeIndex = (std::min)(index, events.size());
    CalyxEngine::NovelDialogueEvent dialogue;
    dialogue.dialogueAssetGuid_ = dialogueAssetGuid;
    dialogue.dialogueLineGuid_ = dialogueLineGuid;

    CalyxEngine::NovelEvent event{
        Guid::New(), CalyxEngine::NovelEventType::Dialogue, dialogue};
    auto iterator = events.insert(events.begin() + safeIndex, std::move(event));
    selectedGuid_ = iterator->guid_;
    saveMessage_ = "未保存の変更があります";
  }

  void InsertEvent(size_t index, CalyxEngine::NovelEventType type) {
    auto &events = asset_.GetEvents();
    const size_t safeIndex = (std::min)(index, events.size());
    CalyxEngine::NovelEvent event{Guid::New(), type,
                                  CalyxEngine::MakeNovelEventData(type)};
    auto iterator = events.insert(events.begin() + safeIndex, std::move(event));
    selectedGuid_ = iterator->guid_;
    saveMessage_ = "未保存の変更があります";
  }

  void InsertSceneObject(size_t index, SceneObject *object) {
    if (!object) {
      return;
    }

    const bool isText =
        dynamic_cast<CalyxEngine::TextSceneObject2d *>(object) != nullptr;
    const bool isSprite =
        dynamic_cast<CalyxEngine::SpriteObject2d *>(object) != nullptr ||
        dynamic_cast<CalyxEngine::SpriteSceneObject2d *>(object) != nullptr;
    if (!isText && !isSprite) {
      saveMessage_ =
          "TextSceneObject2dまたはSprite系オブジェクトだけを追加できます";
      return;
    }

    auto &events = asset_.GetEvents();
    const size_t safeIndex = (std::min)(index, events.size());
    CalyxEngine::NovelSceneObjectEvent clip;
    clip.sceneObjectGuid_ = object->GetGuid();
    clip.advanceMode_ = isText ? CalyxEngine::NovelClipAdvanceMode::Input
                               : CalyxEngine::NovelClipAdvanceMode::Time;

    CalyxEngine::NovelEvent event{
        Guid::New(), CalyxEngine::NovelEventType::SceneObject, clip};
    auto iterator = events.insert(events.begin() + safeIndex, std::move(event));
    selectedGuid_ = iterator->guid_;
    context_.RequestSelectSceneObject(clip.sceneObjectGuid_);
    saveMessage_ = "未保存の変更があります";
  }

  void ApplyPendingMove() {
    if (pendingMoveSource_ < 0 || pendingMoveTarget_ < 0) {
      return;
    }

    auto &events = asset_.GetEvents();
    const size_t source = static_cast<size_t>(pendingMoveSource_);
    const size_t target = static_cast<size_t>(pendingMoveTarget_);
    pendingMoveSource_ = -1;
    pendingMoveTarget_ = -1;

    if (source >= events.size() || target >= events.size() ||
        source == target) {
      return;
    }

    if (source < target) {
      std::rotate(events.begin() + source, events.begin() + source + 1,
                  events.begin() + target + 1);
    } else {
      std::rotate(events.begin() + target, events.begin() + source,
                  events.begin() + source + 1);
    }
    saveMessage_ = "未保存の変更があります";
  }

  void DrawPreview() {
    // Editor preview and in-game playback intentionally share NovelPlayer.
    ImGui::SeparatorText("Viewport再生");
    if (ImGui::Button("プレビュー再生")) {
      player_.SetScene(&asset_);
      player_.Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("停止")) {
      player_.Stop();
    }
    ImGui::SameLine();
    if (ImGui::Button("全文表示 / 次へ")) {
      player_.Next();
    }

    const char *state = player_.IsFinished()  ? "再生完了"
                        : player_.IsPlaying() ? "再生中"
                                              : "停止中";
    ImGui::Text("状態: %s", state);
    ImGui::TextWrapped(
        "会話中は、最初の入力で文字を全文表示し、次の入力で次へ進みます。");
    ImGui::TextDisabled(
        "シーンオブジェクトのClipは通常のScene Viewportへ表示されます。");
  }

  void DrawInspector() {
    auto *event = asset_.FindEvent(selectedGuid_);
    if (!event) {
      ImGui::TextDisabled("タイムラインから編集するClipを選択してください。");
      return;
    }

    ImGui::SeparatorText("Clip設定");
    int type = static_cast<int>(event->type_);
    const char *names[] = {"会話",       "画像を表示", "画像を非表示",
                           "背景を変更", "待機",       "シーンオブジェクト"};
    if (ImGui::Combo("イベント種別", &type, names, 6)) {
      event->type_ = static_cast<CalyxEngine::NovelEventType>(type);
      event->data_ = CalyxEngine::MakeNovelEventData(event->type_);
    }

    using Type = CalyxEngine::NovelEventType;
    switch (event->type_) {
    case Type::Dialogue:
      DrawDialogue(*event);
      break;
    case Type::ShowImage:
      DrawImage(*event);
      break;
    case Type::HideImage:
      DrawHideImage(*event);
      break;
    case Type::ChangeBackground:
      DrawBackground(*event);
      break;
    case Type::Wait:
      ImGui::DragFloat(
          "待機時間",
          &std::get<CalyxEngine::NovelWaitEvent>(event->data_).duration_, 0.05f,
          0.0f, 600.0f);
      break;
    case Type::SceneObject:
      DrawSceneObjectClip(*event);
      break;
    }
  }

  void DrawSceneObjectClip(CalyxEngine::NovelEvent &event) {
    auto &data = std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_);
    const auto *resolver = CalyxEngine::GetCurrentSceneObjectResolver();
    const auto object =
        resolver ? resolver->ResolveSceneObject(data.sceneObjectGuid_)
                 : nullptr;

    ImGui::Text("対象オブジェクト: %s", object
                                            ? object->GetDisplayName().c_str()
                                            : "(対象が見つかりません)");
    ImGui::TextDisabled("対象を追加するには、HierarchyのTextまたはSpriteを"
                        "タイムライン上の「＋」へドロップします。");

    int advanceMode =
        data.advanceMode_ == CalyxEngine::NovelClipAdvanceMode::Time ? 0 : 1;
    const char *advanceNames[] = {"時間で進む", "ボタン入力で進む"};
    if (ImGui::Combo("進行方法", &advanceMode, advanceNames, 2)) {
      data.advanceMode_ = advanceMode == 0
                              ? CalyxEngine::NovelClipAdvanceMode::Time
                              : CalyxEngine::NovelClipAdvanceMode::Input;
    }
    if (data.advanceMode_ == CalyxEngine::NovelClipAdvanceMode::Time) {
      ImGui::DragFloat("表示時間", &data.duration_, 0.05f, 0.0f, 600.0f,
                       "%.2f 秒");
    }
    ImGui::Checkbox("完了時に非表示", &data.hideOnComplete_);
  }

  void DrawTextSettings() {
    ImGui::SeparatorText("シーン共通テキスト設定");
    auto &settings = asset_.GetTextSettings();
    CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Font,
                                                 &settings.fontGuid_);
    ImGui::TextDisabled(
        "選択したフォントは話者名と会話本文の両方へ適用されます。");
    ImGui::DragFloat2("本文の位置", &settings.dialoguePosition_.x, 1.0f);
    ImGui::DragFloat2("本文の領域サイズ", &settings.dialogueSize_.x, 1.0f, 1.0f,
                      4096.0f);
    ImGui::DragFloat2("話者名の位置", &settings.speakerPosition_.x, 1.0f);
    ImGui::DragFloat2("話者名の領域サイズ", &settings.speakerSize_.x, 1.0f,
                      1.0f, 4096.0f);
    DrawTextStyle("本文スタイル", settings.dialogueStyle_);
    DrawTextStyle("話者名スタイル", settings.speakerStyle_);
  }

  static void DrawTextStyle(const char *label, CalyxEngine::TextStyle &style) {
    if (!ImGui::TreeNode(label)) {
      return;
    }
    ImGui::DragFloat("文字サイズ", &style.fontSize_, 1.0f, 1.0f, 256.0f);
    ImGui::ColorEdit4("文字色", &style.color_.x);
    ImGui::DragFloat("文字間隔", &style.letterSpacing_, 0.1f, -20.0f, 100.0f);
    ImGui::DragFloat("行間", &style.lineSpacing_, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("最大幅", &style.maxWidth_, 1.0f, 0.0f, 4096.0f);
    ImGui::Checkbox("自動折り返し", &style.wordWrap_);
    ImGui::TreePop();
  }

  void DrawDialogue(CalyxEngine::NovelEvent &event) {
    auto &data = std::get<CalyxEngine::NovelDialogueEvent>(event.data_);
    DrawAssetSelector("会話アセット", AssetType::Dialogue,
                      data.dialogueAssetGuid_);
    CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Dialogue,
                                                 &data.dialogueAssetGuid_);

    const AssetRecord *record =
        data.dialogueAssetGuid_.isValid()
            ? AssetDatabase::GetInstance()->Get(data.dialogueAssetGuid_)
            : nullptr;
    if (!record || !dialogueCache_.Load(record->sourcePath)) {
      ImGui::TextDisabled("先に会話アセットを選択してください。");
      return;
    }

    const std::string preview = DialogueLabel(data.dialogueLineGuid_);
    if (ImGui::BeginCombo("セリフ", preview.c_str())) {
      int index = 1;
      for (const auto &line : dialogueCache_.GetLines()) {
        const std::string label =
            std::to_string(index++) + " " + line.speaker_ + " " + line.text_;
        if (ImGui::Selectable(label.c_str(),
                              line.guid_ == data.dialogueLineGuid_)) {
          data.dialogueLineGuid_ = line.guid_;
        }
      }
      ImGui::EndCombo();
    }
  }

  void DrawImage(CalyxEngine::NovelEvent &event) {
    auto &data = std::get<CalyxEngine::NovelShowImageEvent>(event.data_);
    DrawAssetSelector("テクスチャ", AssetType::Texture, data.textureGuid_);
    CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Texture,
                                                 &data.textureGuid_);
    ImGui::DragFloat2("表示位置", &data.position_.x);
    ImGui::DragFloat2("表示倍率", &data.scale_.x, 0.01f);
  }

  void DrawBackground(CalyxEngine::NovelEvent &event) {
    auto &data = std::get<CalyxEngine::NovelBackgroundEvent>(event.data_);
    DrawAssetSelector("背景テクスチャ", AssetType::Texture, data.textureGuid_);
    CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Texture,
                                                 &data.textureGuid_);
  }

  // Asset selection is available as a combo in addition to drag and drop.
  // This makes the editor usable even when the Asset panel is closed.
  void DrawAssetSelector(const char *label, AssetType type, Guid &guid) {
    const AssetRecord *currentRecord =
        guid.isValid() ? AssetDatabase::GetInstance()->Get(guid) : nullptr;
    const std::string currentName =
        currentRecord ? currentRecord->sourcePath.filename().string()
                      : "(未選択)";

    if (!ImGui::BeginCombo(label, currentName.c_str())) {
      return;
    }

    for (const AssetRecord *record : AssetDatabase::GetInstance()->GetView()) {
      if (!record || record->type != type) {
        continue;
      }

      const std::string filename = record->sourcePath.filename().string();
      if (ImGui::Selectable(filename.c_str(), record->guid == guid)) {
        guid = record->guid;
      }
    }
    ImGui::EndCombo();
  }

  void DrawHideImage(CalyxEngine::NovelEvent &event) {
    auto &data = std::get<CalyxEngine::NovelHideImageEvent>(event.data_);
    const std::string current =
        data.targetGuid_.isValid() ? data.targetGuid_.ToString() : "(未選択)";
    if (!ImGui::BeginCombo("非表示にする画像", current.c_str())) {
      return;
    }

    for (const auto &candidate : asset_.GetEvents()) {
      if (candidate.type_ != CalyxEngine::NovelEventType::ShowImage) {
        continue;
      }
      const std::string guid = candidate.guid_.ToString();
      if (ImGui::Selectable(guid.c_str(),
                            candidate.guid_ == data.targetGuid_)) {
        data.targetGuid_ = candidate.guid_;
      }
    }
    ImGui::EndCombo();
  }

  void Add(CalyxEngine::NovelEventType type) {
    selectedGuid_ = asset_.AddEvent(type).guid_;
    saveMessage_ = "未保存の変更があります";
  }

  // Shows the identifying value directly in the playback-order list.
  std::string EventSummary(const CalyxEngine::NovelEvent &event, int index) {
    const std::string prefix =
        std::to_string(index) + ". " + EventTypeName(event.type_);

    if (event.type_ == CalyxEngine::NovelEventType::SceneObject) {
      const auto &data =
          std::get<CalyxEngine::NovelSceneObjectEvent>(event.data_);
      const auto *resolver = CalyxEngine::GetCurrentSceneObjectResolver();
      const auto object =
          resolver ? resolver->ResolveSceneObject(data.sceneObjectGuid_)
                   : nullptr;
      return object ? prefix + " | " + object->GetDisplayName()
                    : prefix + " | (object missing)";
    }

    if (event.type_ == CalyxEngine::NovelEventType::Dialogue) {
      const auto &data = std::get<CalyxEngine::NovelDialogueEvent>(event.data_);
      const AssetRecord *record =
          data.dialogueAssetGuid_.isValid()
              ? AssetDatabase::GetInstance()->Get(data.dialogueAssetGuid_)
              : nullptr;
      CalyxEngine::DialogueAsset dialogue;
      if (record && dialogue.Load(record->sourcePath)) {
        const auto *line = dialogue.FindLine(data.dialogueLineGuid_);
        if (line) {
          return prefix + " | " + line->speaker_ + ": " + line->text_;
        }
      }
      return prefix + " | (line not selected)";
    }

    Guid assetGuid{};
    if (event.type_ == CalyxEngine::NovelEventType::ShowImage) {
      assetGuid =
          std::get<CalyxEngine::NovelShowImageEvent>(event.data_).textureGuid_;
    }
    if (event.type_ == CalyxEngine::NovelEventType::ChangeBackground) {
      assetGuid =
          std::get<CalyxEngine::NovelBackgroundEvent>(event.data_).textureGuid_;
    }
    if (assetGuid.isValid()) {
      const AssetRecord *record = AssetDatabase::GetInstance()->Get(assetGuid);
      if (record) {
        return prefix + " | " + record->sourcePath.filename().string();
      }
    }

    if (event.type_ == CalyxEngine::NovelEventType::Wait) {
      const float duration =
          std::get<CalyxEngine::NovelWaitEvent>(event.data_).duration_;
      return prefix + " | " + std::to_string(duration) + " sec";
    }
    return prefix;
  }

  void CopyName(const std::string &name) {
    assetName_.fill('\0');
    const size_t count = (std::min)(name.size(), assetName_.size() - 1);
    std::memcpy(assetName_.data(), name.data(), count);
  }

  void Save() {
    const std::string name = assetName_.data();
    if (name.empty()) {
      saveMessage_ = "保存する前にアセット名を入力してください";
      return;
    }
    auto *database = AssetDatabase::GetInstance();
    const auto path =
        database->GetRoot() / "Novel" / "Scenes" / (name + ".novelscene");
    if (asset_.Save(path)) {
      currentPath_ = path;
      database->RegisterOrUpdate(path, AssetType::NovelScene);
      saveMessage_ = "保存完了: " + path.filename().string();
    } else {
      saveMessage_ = "保存に失敗しました";
    }
  }

  std::string DialogueLabel(const Guid &guid) const {
    const auto *line = dialogueCache_.FindLine(guid);
    return line ? line->speaker_ + " " + line->text_ : "(未選択)";
  }

  bool open_ = true;
  CalyxEditor::EditorToolContext context_;
  CalyxEngine::NovelSceneAsset asset_;
  CalyxEngine::DialogueAsset dialogueCache_;
  CalyxEngine::NovelPlayer player_;
  Guid selectedGuid_{};
  std::filesystem::path currentPath_;
  std::array<char, 128> assetName_{};
  std::chrono::steady_clock::time_point lastUpdate_{};
  std::string saveMessage_;
  size_t insertionIndex_ = 0;
  int pendingMoveSource_ = -1;
  int pendingMoveTarget_ = -1;
  bool requestInsertPopup_ = false;
  float timelineZoom_ = 120.0f;
};

CalyxEditor::IEditorTool *
CreateGameCameraEditor(const CalyxEditor::EditorToolContext &context) {
  return new GameCameraEditor(context);
}

CalyxEditor::IEditorTool *
CreateDialogueEditor(const CalyxEditor::EditorToolContext &context) {
  return new DialogueEditor(context);
}

CalyxEditor::IEditorTool *
CreateNovelSceneEditor(const CalyxEditor::EditorToolContext &context) {
  return new NovelSceneEditor(context);
}

void DestroyEditorTool(CalyxEditor::IEditorTool *tool) {
  delete tool;
}
} // namespace

extern "C" __declspec(dllexport) bool
RegisterCalyxEditorTools(std::uint32_t apiVersion,
                         CalyxEditor::IEditorHost *host) {
  if (!host || apiVersion != CalyxEditor::kEditorToolApiVersion)
    return false;

  CalyxEditor::EditorToolDescriptor camera;
  camera.id = "Game.CameraEditor";
  camera.displayName = "Game Camera Editor";
  camera.menuPath = "Game/Camera";
  camera.workspaceId = "Game.Camera";
  camera.layoutPath = "GameCameraEditor.ini";
  camera.create = &CreateGameCameraEditor;
  camera.destroy = &DestroyEditorTool;

  CalyxEditor::EditorToolDescriptor dialogue;
  dialogue.id = "Novel.DialogueEditor";
  dialogue.displayName = "Dialogue Editor";
  dialogue.menuPath = "Novel/Dialogue Editor";
  dialogue.workspaceId = "Novel.Dialogue";
  dialogue.layoutPath = "DialogueEditor.ini";
  dialogue.create = &CreateDialogueEditor;
  dialogue.destroy = &DestroyEditorTool;

  CalyxEditor::EditorToolDescriptor scene;
  scene.id = "Novel.SceneEditor";
  scene.displayName = "Novel Scene Editor";
  scene.menuPath = "Novel/Scene Editor";
  scene.workspaceId = "Novel.Scene";
  scene.layoutPath = "NovelSceneEditor.ini";
  scene.create = &CreateNovelSceneEditor;
  scene.destroy = &DestroyEditorTool;

  const bool cameraRegistered = host->RegisterTool(camera);
  const bool dialogueRegistered = host->RegisterTool(dialogue);
  const bool sceneRegistered = host->RegisterTool(scene);
  return cameraRegistered && dialogueRegistered && sceneRegistered;
}
