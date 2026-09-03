#pragma once

// engine
#include <Engine/Foundation/Math/Vector3.h>

// game
#include <Game/Floater/ChainMemberData.h>

// std
#include <vector>


namespace ResultCarry {

	/// ゲーム開始時に抽選した、隕石の落下地点
	struct FallSpot {
		CalyxEngine::Vector3 pos{};
		float delay = 0.0f;   // リザルト開始から落ち始めるまで
	};

	inline std::vector<FallSpot>        fallSpots;
	inline std::vector<ChainMemberData> chain;      // [0] は自機自身
	inline CalyxEngine::Vector3         playerPos{};
	inline float                        playerYaw = 0.0f;

	inline void Clear() {
		fallSpots.clear();
		chain.clear();
		playerPos = {};
		playerYaw = 0.0f;
	}

};
