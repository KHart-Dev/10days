#include <CalyxEngine/EditorExtension.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <externals/imgui/imgui.h>

#include <numbers>

namespace {
	class GameCameraEditor final : public CalyxEditor::IEditorTool {
	public:
		explicit GameCameraEditor(const CalyxEditor::EditorToolContext& context) : context_(context) {}
		void OnOpen() override { open_ = true; SyncFromCamera(); }
		void Draw() override {
			if(!open_) return;
			ImGui::Begin("Game Camera Editor###Game.CameraEditor", &open_);
			auto* camera = context_.GetMainCamera();
			if(!camera) { ImGui::TextDisabled("No main camera is available."); ImGui::End(); return; }
			if(context_.IsPlaying()) ImGui::TextDisabled("Editing the runtime camera");
			bool changed = ImGui::DragFloat3("Position", position_, 0.1f);
			changed |= ImGui::DragFloat3("Rotation", rotation_, 0.01f);
			changed |= ImGui::DragFloat("Field of View", &fieldOfView_, 0.1f, 1.0f, 179.0f);
			if(changed) {
				camera->SetCamera({position_[0], position_[1], position_[2]}, {rotation_[0], rotation_[1], rotation_[2]});
				camera->SetFovY(fieldOfView_ * std::numbers::pi_v<float> / 180.0f);
				camera->UpdateMatrix();
			}
			if(!context_.IsPlaying() && ImGui::Button("Save Scene")) context_.RequestSaveScene();
			ImGui::End();
		}
		bool IsOpen() const override { return open_; }
	private:
		void SyncFromCamera() {
			if(auto* camera = context_.GetMainCamera()) {
				const auto& p = camera->GetTranslate(); const auto& r = camera->GetRotate();
				position_[0] = p.x; position_[1] = p.y; position_[2] = p.z;
				rotation_[0] = r.x; rotation_[1] = r.y; rotation_[2] = r.z;
				fieldOfView_ = camera->GetFovY() * 180.0f / std::numbers::pi_v<float>;
			}
		}
		CalyxEditor::EditorToolContext context_;
		bool open_ = true;
		float position_[3]{};
		float rotation_[3]{};
		float fieldOfView_ = 60.0f;
	};

	CalyxEditor::IEditorTool* CreateGameCameraEditor(const CalyxEditor::EditorToolContext& context) {
		return new GameCameraEditor(context);
	}
	void DestroyGameCameraEditor(CalyxEditor::IEditorTool* tool) { delete tool; }
}

extern "C" __declspec(dllexport) bool RegisterCalyxEditorTools(
	std::uint32_t apiVersion, CalyxEditor::IEditorHost* host) {
	if(!host || apiVersion != CalyxEditor::kEditorToolApiVersion) return false;
	CalyxEditor::EditorToolDescriptor descriptor;
	descriptor.id = "Game.CameraEditor";
	descriptor.displayName = "Game Camera Editor";
	descriptor.menuPath = "Game/Camera";
	descriptor.workspaceId = "Game.Camera";
	descriptor.layoutPath = "GameCameraEditor.ini";
	descriptor.create = &CreateGameCameraEditor;
	descriptor.destroy = &DestroyGameCameraEditor;
	return host->RegisterTool(descriptor);
}
