#pragma once
#include "entt.hpp"
#include "Engine/Core/PhysX.h"

#include <DirectXMath.h>

namespace physx { class PxScene; }

// レイキャストの結果
struct RaycastHitResult
{
	bool               hit = false;      // 当たったかどうか
	DirectX::XMFLOAT3  position{};       // 当たった座標
	DirectX::XMFLOAT3  normal{};         // 当たった面の法線
	float              distance = 0.0f;  // 開始位置からの距離
};

// レイキャストデバッグ描画
struct DebugRay
{
	DirectX::XMFLOAT3 start; // 始点
	DirectX::XMFLOAT3 end;   // 終点
	DirectX::XMFLOAT4 color; // 色
};

class PhysicsSystem
{
public:
	// 更新
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime);

	// 剛体コンポーネントが外れる/エンティティが破壊される前にPhysXの実体を削除する
	static void ConnectDestroyListeners(entt::registry& registry);

	// レイヤーにレイキャスト
	static RaycastHitResult Raycast(
		physx::PxScene* pxScene,
		const DirectX::XMFLOAT3& origin,
		const DirectX::XMFLOAT3& direction,
		float maxDistance,
		physx::PxU32 wantedLayers);
	// デバッグ表示用
	static std::vector<DebugRay> FetchDebugRays();

private:

	// PhysicsBodyTypeComponent::type に合わせて、剛体コンポーネントを付け外しする
	void SyncBodyTypes(entt::registry& registry);

	// 削除予約されたActorを解放する
	void ProcessDestroyQueue();

	// 未生成のStaticBodyComponent生成
	void ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

	// 未生成のCharacterControllerComponent生成+CharacterController移動処理
	void ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime);

	// 未生成の
	// void ProcessDynamicBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

private:
	// レイの記録
	static std::vector<DebugRay> debugRays;
};