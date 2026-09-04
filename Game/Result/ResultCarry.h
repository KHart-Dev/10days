#pragma once

// game
#include <Game/Floater/ChainMemberData.h>

// std
#include <vector>


namespace ResultCarry {

	inline std::vector<ChainMemberData> chain;      // [0] は自機自身

	/// どのステージからリザルトへ来たか。落下地点をステージ別に読むのに使う
	inline int stageIndex = 0;

	inline void Clear() {
		chain.clear();
		stageIndex = 0;
	}

};
