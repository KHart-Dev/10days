#include "Thorn.h"

Thorn::Thorn() :Actor("debugCube.obj", "thorn") {
	//初期化時にスケールを小さくしておく
	worldTransform_.scale *= 0.5f;
}
Thorn::~Thorn() = default;

