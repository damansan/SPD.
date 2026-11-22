#pragma once
#include <PxPhysicsAPI.h>

class PhysX
{
public:
	PhysX() = default;
	~PhysX() = default;

	PhysX(const PhysX&) = delete;                // コピーコンストラクタ禁止
	PhysX& operator=(const PhysX&) = delete;     // コピー代入禁止
	PhysX(PhysX&&) noexcept = delete;            // ムーブコンストラクタ禁止
	PhysX& operator=(PhysX&&) noexcept = delete; // ムーブ代入禁止

	// 初期化
	void Initialize();
	// 終了処理
	void Cleanup();

	// PxPhysics取得
	physx::PxPhysics* GetPhysics() { return physics; }
	// CPUディスパッチャ取得
	physx::PxDefaultCpuDispatcher* GetDispatcher() { return dispatcher; }
	// デフォルトマテリアル取得
	physx::PxMaterial* GetDefaultMaterial() { return defaultMaterial; }

	// メッシュデータからPxTriangleMeshを生成
	physx::PxTriangleMesh* CreateTriangleMesh(
		const void* vertexData, uint32_t vertexCount, uint32_t vertexStride,
		const uint32_t* indices, uint32_t triangleCount);

private:
	physx::PxDefaultAllocator      allocator;        // アロケータ
	physx::PxDefaultErrorCallback  errorCallback;    // エラーコールバック

	physx::PxPvd* pvd = nullptr;                     // PhysXVisualDebugger接続
	physx::PxPvdTransport* pvdTransport = nullptr;   // PhysXVisualDebugger接続用トランスポート

	physx::PxFoundation* foundation = nullptr;
	physx::PxPhysics* physics = nullptr;
	physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
	physx::PxMaterial* defaultMaterial = nullptr;

};