#pragma once

// engine
#include <Engine/Foundation/Math/Vector3.h>

/// <summary>人の体の点と、角度まわりの小物</summary>
namespace BodyNode {

	/// <summary>繋がれるのは手の2点だけ</summary>
	inline constexpr int kHandCount = 2;

	/// <summary>0 = 左手、1 = 右手</summary>
	inline const CalyxEngine::Vector3 kHand[kHandCount] = {
		{ -1.0f, 0.0f, 0.0f },
		{  1.0f, 0.0f, 0.0f },
	};

	/// <summary>体のローカルで、その手が生えている向き</summary>
	float HandAngle(int hand);

	/// <summary>-π〜π へ丸めた角度</summary>
	float WrapAngle(float rad);

	/// <summary>Y軸まわりに回す</summary>
	CalyxEngine::Vector3 RotateY(const CalyxEngine::Vector3& v, float yaw);

	/// <summary>点と線分の距離の2乗。XZ平面で見る</summary>
	float DistanceSqToSegment(const CalyxEngine::Vector3& p,
							  const CalyxEngine::Vector3& a,
							  const CalyxEngine::Vector3& b);
}
