#include "PhysX.h"

using namespace physx;

// 初期化
void PhysX::Initialize()
{
	// Foundatio作成
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

#ifdef _DEBUG
	// PhysXVisualDebuggerに接続
	pvd = PxCreatePvd(*foundation);
	pvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	pvd->connect(*pvdTransport, physx::PxPvdInstrumentationFlag::eALL);
#endif

	// PxPhysics本体作成
	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);

	// シミュレーションをCPUスレッドに割り振るディスパッチャを作成
	dispatcher = PxDefaultCpuDispatcherCreate(1); // スレッド数１(夏期講座だと2スレッドだった)

	// デフォルトマテリアル(静摩擦, 動摩擦, 反発係数)作成
	defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.6f);
}
// 終了処理
void PhysX::Cleanup()
{
	// 作成した順と逆順に解放する
	if (defaultMaterial) { defaultMaterial->release(); defaultMaterial = nullptr; }
	if (dispatcher)      { dispatcher->release();      dispatcher      = nullptr; }
	if (physics)         { physics->release();         physics         = nullptr; }
	if (pvd)             { pvd->disconnect();
				           pvd->release();             pvd             = nullptr; }
	if (pvdTransport)    { pvdTransport->release();    pvdTransport    = nullptr; }
	if (foundation)      { foundation->release();      foundation      = nullptr; }
}

// メッシュデータからPxTriangleMeshを生成
physx::PxTriangleMesh* PhysX::CreateTriangleMesh(
	const void* vertexData, 
	uint32_t vertexCount,
	uint32_t vertexStride,
	const uint32_t* indices,
	uint32_t triangleCount)
{
	// メッシュ情報組み立て
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = vertexCount;
	meshDesc.points.stride = vertexStride;
	meshDesc.points.data = vertexData;

	meshDesc.triangles.count = triangleCount;
	meshDesc.triangles.stride = 3 * sizeof(uint32_t);
	meshDesc.triangles.data = indices;

	// Cookingパラメータ
	PxCookingParams cookingParams(physics->getTolerancesScale());

	// メッシュ生成
	return PxCreateTriangleMesh(
		cookingParams,
		meshDesc,
		physics->getPhysicsInsertionCallback()
	);
}
