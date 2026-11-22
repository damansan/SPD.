#include "Scene.h"

// 物理シーンの作成
void Scene::InitializePhysicsScene(PhysX& physX)
{
	// シーン組み立て
	physx::PxSceneDesc sceneDesc(physX.GetPhysics()->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = physX.GetDispatcher();
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

	pxScene = physX.GetPhysics()->createScene(sceneDesc);

	// PhysXVisualDebuggerに送る情報を設定
	if (physx::PxPvdSceneClient* pvdClient = pxScene->getScenePvdClient())
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS,  true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS,     true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	// キャラクターコントローラーマネージャー作成
	controllerManager = PxCreateControllerManager(*pxScene);
}

// 物理シーンの破棄
void Scene::FinalizePhysicsScene()
{
	if (controllerManager)
	{
		controllerManager->release();
		controllerManager = nullptr;
	}

	if (pxScene)
	{
		pxScene->release();
		pxScene = nullptr;
	}
}

// 物理シーンの更新
void Scene::UpdatePhysicsScene(float elapsedTime)
{
	pxScene->simulate(elapsedTime);
	pxScene->fetchResults(true);
}