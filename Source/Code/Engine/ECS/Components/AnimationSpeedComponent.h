#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

struct AnimationSpeedComponent
{
	REFLECT_BEGIN(AnimationSpeedComponent, "Animation Speed")
		
		REFLECT_FIELD(std::string, animationSpeedPath, (""), ("Speed Curve File", 0.0f, 0.0f, 0.1f, "ANIMSPEED_PATH")) // .animspeedファイルのパス
	
	REFLECT_END()
};