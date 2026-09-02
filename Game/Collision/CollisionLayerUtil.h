#pragma once

#include <Engine/Collision/CollisionTypes.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <optional>
#include <string_view>

namespace GameCollision {
	inline std::optional<CollisionLayerId> FindLayerId(std::string_view name) {
		const auto& layers = SceneAPI::GetCollisionLayerSettings().GetLayers();
		for (const auto& layer : layers) {
			if (layer.name == name) {
				return layer.id;
			}
		}
		return std::nullopt;
	}
}
