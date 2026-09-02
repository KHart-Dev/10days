#include "BodyNode.h"

// engine
#include <Engine/Foundation/Math/MathUtil.h>

// std
#include <algorithm>
#include <cmath>

namespace BodyNode {

	float HandAngle(int hand) {
		return std::atan2f(kHand[hand].x, kHand[hand].z);
	}

	float WrapAngle(float rad) {

		rad = std::fmodf(rad + CalyxEngine::kPi, CalyxEngine::kTwoPi);
		if (rad < 0.0f) {
			rad += CalyxEngine::kTwoPi;
		}
		return rad - CalyxEngine::kPi;
	}

	CalyxEngine::Vector3 RotateY(const CalyxEngine::Vector3& v, float yaw) {
		return CalyxEngine::TransformNormal(v, CalyxEngine::MakeRotateYMatrix(yaw));
	}

	float DistanceSqToSegment(const CalyxEngine::Vector3& p,
							  const CalyxEngine::Vector3& a,
							  const CalyxEngine::Vector3& b) {

		const CalyxEngine::Vector3 ab{ b.x - a.x, 0.0f, b.z - a.z };
		const CalyxEngine::Vector3 ap{ p.x - a.x, 0.0f, p.z - a.z };

		const float lenSq = CalyxEngine::Vector3::Dot(ab, ab);
		const float t = (lenSq > 0.0f)
			? std::clamp(CalyxEngine::Vector3::Dot(ap, ab) / lenSq, 0.0f, 1.0f)
			: 0.0f;

		const CalyxEngine::Vector3 d = ap - ab * t;
		return CalyxEngine::Vector3::Dot(d, d);
	}
}
