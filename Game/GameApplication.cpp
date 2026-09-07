#include "GameApplication.h"

// game
#include <Game/Result/ResultCarry.h>

// engine
#include <CalyxEngine/Engine.h>

void GameApplication::OnInitialize() {
	ResultCarry::Clear();

	Calyx::SetWindowTitle("4004_かけろ！にんげんブリッジ");

}

void GameApplication::OnUpdate() {}

void GameApplication::OnRender() {}

void GameApplication::OnFinalize() {}
