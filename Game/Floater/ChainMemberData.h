#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <Game/Floater/BodyNode.h>

struct ChainMemberData {
	CalyxEngine::Vector3 offset{};
	float localAngle = 0.0f;
	CalyxEngine::Vector3 handLocal[BodyNode::kHandCount]{};
	int parent = -1;
	int parentHand = 0;
	int joinHand = 0;
};