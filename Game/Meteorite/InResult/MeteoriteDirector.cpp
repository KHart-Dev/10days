#include "MeteoriteDirector.h"

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

// std
#include <algorithm>

MeteoriteDirector::MeteoriteDirector()
	: Actor("cone.obj", "MeteoriteDirector") {}

void MeteoriteDirector::Initialize() {
	Actor::Initialize();

	param_.ownerGuid_ = GetGuid();
	param_.LoadParams();

	DisableGravity();
	SetDrawEnable(false);
}

void MeteoriteDirector::Update(float dt) {

	// 初回 Update で集める
	if (!collected_) {
		CollectWarnings();
		collected_ = true;
	}

	if (!started_) {
		timer_ += dt;
		if (timer_ >= param_.startDelay) {
			StartAll();
			started_ = true;
		}
	}

	Actor::Update(dt);
}

void MeteoriteDirector::CollectWarnings() {

	warnings_.clear();

	SceneContext* context = SceneContext::Current();
	if (!context) {
		return;
	}

	SceneObjectLibrary* library = context->GetObjectLibrary();
	if (!library) {
		return;
	}

	// 型で探す
	for (const std::shared_ptr<SceneObject>& object : library->GetAllObjectsShared()) {
		if (std::shared_ptr<MeteoriteWarning> warning =
			std::dynamic_pointer_cast<MeteoriteWarning>(object)) {
			warnings_.push_back(std::move(warning));
		}
	}
}

void MeteoriteDirector::StartAll() {

	// 落ちる順番は各 Warning の delay が持つ
	for (const std::shared_ptr<MeteoriteWarning>& warning : warnings_) {
		if (warning) {
			warning->Start(param_.settings);
		}
	}
}

bool MeteoriteDirector::IsFinished() const {

	if (!started_) {
		return false;
	}

	return std::all_of(warnings_.begin(), warnings_.end(),
		[](const std::shared_ptr<MeteoriteWarning>& warning) {
			return !warning || warning->IsFinished();
		});
}

void MeteoriteDirector::DisableGravity() {
	auto& movement = GetCharacterMovement();
	movement.SetGravity(0.0f);
	movement.SetMaxFallSpeed(0.0f);
	movement.SetFloorProbeDistance(0.0f);
	movement.SetFloorSnapDistance(0.0f);
}

void MeteoriteDirector::DerivativeGui() {

	ImGui::Text("Warnings: %d", static_cast<int>(warnings_.size()));
	ImGui::Text("Started : %s", started_ ? "yes" : "no");
	ImGui::Text("Finished: %s", IsFinished() ? "yes" : "no");
	param_.ShowGui();
}
