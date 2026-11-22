#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

struct MeshComponent
{
	REFLECT_BEGIN(MeshComponent, "Mesh")
	
		// モデルファイルパス
		REFLECT_FIELD(std::string, modelPath, (""), ("Model", 0.0f, 0.0f, 0.1f, "MODEL_PATH")) 
		
	REFLECT_END()

		std::string previousPath; // 前フレームのパスを記録

	MeshComponent() = default;
	MeshComponent(const std::string& path) : modelPath(path) {}
};