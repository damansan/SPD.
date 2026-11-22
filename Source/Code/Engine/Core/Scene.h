#pragma once
#include "entt.hpp"
#include "SceneContext.h"
#include "PhysX.h"
#include "Engine/ECS/Systems/MeshLoaderSystem.h"

class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	Scene(const Scene&) = delete;                // コピーコンストラクタ禁止
	Scene& operator=(const Scene&) = delete;     // コピー代入禁止
	Scene(Scene&&) noexcept = delete;            // ムーブコンストラクタ禁止
	Scene& operator=(Scene&&) noexcept = delete; // ムーブ代入禁止

	// 初期化
	virtual void Initialize(const SceneContext& context) = 0;
	// 終了化
	virtual void Finalize() = 0;
	// 更新
	virtual void Update(float elapsedTime, const SceneContext& context) = 0;
	// 描画
	virtual void Render(const SceneContext& context) = 0;

	// レジストリ取得
	entt::registry& GetRegistry() { return registry; }
	// 物理シーン取得
	physx::PxScene* GetPhysicsScene() { return pxScene; }
	// キャラクターコントローラーマネージャー取得
	physx::PxControllerManager* GetControllerManager() { return controllerManager; }

protected:

	// Entity/Component管理
	entt::registry registry; 

	// 物理シーンの作成
	void InitializePhysicsScene(PhysX& physX);
	// 物理シーンの破棄
	void FinalizePhysicsScene();
	// 物理シーンの更新
	void UpdatePhysicsScene(float elapsedTime);
	// 物理ワールド本体
	physx::PxScene* pxScene = nullptr; 
	// キャラクターコントローラーマネージャー
	physx::PxControllerManager* controllerManager = nullptr;
};