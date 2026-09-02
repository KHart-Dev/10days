#pragma once

#include <Engine/Scene/Utility/SceneUtility.h>

#include <string>

/*-----------------------------------------------------------------------------------------
 * GameAudio
 * - ゲームで鳴らす音の名前と、BGMの切り替えをまとめたもの
 *---------------------------------------------------------------------------------------*/
namespace GameAudio {

	// BGM
	inline constexpr const char* kBgmTitle = "titleBGM.mp3";
	inline constexpr const char* kBgmStory = "storyBGM.mp3";
	inline constexpr const char* kBgmGame = "gameBGM.mp3";
	inline constexpr const char* kBgmResult = "resultBGM.mp3";

	// SE
	inline constexpr const char* kSeConnect = "connectSE.mp3";
	inline constexpr const char* kSeDamage = "damageSE.mp3";
	inline constexpr const char* kSeMove = "moveSE.mp3";
	inline constexpr const char* kSeGoal = "goalSE.mp3";
	inline constexpr const char* kSeCount = "countSE.mp3";

	// リザルトSE


	// 既定音量
	inline constexpr float kBgmVolume = 0.3f;
	inline constexpr float kSeVolume = 0.3f;

	namespace Detail {
		inline std::string currentBgm; // 今鳴らしているBGM。空なら無し
		inline std::string currentSe;
	}


	inline void PlaySe(const std::string& filename, float volume = kSeVolume) {
		AudioAPI::Play(filename, false, volume);
	}

	inline void PlaySeLoop(const std::string& filename, float volume = kSeVolume) {
		if (Detail::currentSe == filename) {
			return;
		}
		AudioAPI::Play(filename, true, volume);
		Detail::currentSe = filename;
	}

	inline void StopSe() {
		if (Detail::currentSe.empty()) {
			return;
		}
		AudioAPI::Stop(Detail::currentSe);
		Detail::currentSe.clear();
	}


	/// BGMを切り替える。同じ曲が鳴っているなら何もしない(エリア移動で鳴り直さない)
	inline void PlayBgm(const std::string& filename, float volume = kBgmVolume) {
		if (Detail::currentBgm == filename) {
			return;
		}
		// 一度も再生していない曲を止めようとすると engine 側で assert するので、鳴らした曲だけ止める
		if (!Detail::currentBgm.empty()) {
			AudioAPI::Stop(Detail::currentBgm);
		}
		AudioAPI::Play(filename, true, volume);
		Detail::currentBgm = filename;
	}

	inline void StopBgm() {
		if (Detail::currentBgm.empty()) {
			return;
		}
		AudioAPI::Stop(Detail::currentBgm);
		Detail::currentBgm.clear();
	}
}
