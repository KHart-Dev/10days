#pragma once

// engine
#include <Engine/Objects/3D/Actor/Actor.h>

/*-----------------------------------------------------------------------------------------
 * Thorn
 * - 棘
 * - 障害物の周りについている棘
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Thorn", Icon = "UI/Tool/cube.dds"
			,Placeable = false,PrefabEditable = true,PrefabRoot = true)
class Thorn final
	:public Actor {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	Thorn();
	~Thorn()override;

	std::string_view GetObjectClassName() const override { return "Thorn"; }

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
};

