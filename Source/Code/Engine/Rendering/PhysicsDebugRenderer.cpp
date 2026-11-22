#include "PhysicsDebugRenderer.h"
#include "Engine/ECS/Systems/PhysicsSystem.h"
#include <string>
#include <algorithm>

void PhysicsDebugRenderer::Initialize(ID3D11Device* device)
{
	this->device = device;
	primitiveRenderer = PrimitiveRenderer(device);
	shapeRenderer = ShapeRenderer(device);

	// 半透明ブレンドステート
	{
		D3D11_BLEND_DESC desc = {};
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		device->CreateBlendState(&desc, blendState.GetAddressOf());
	}
	// 深度テスト（書き込みあり）
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		device->CreateDepthStencilState(&desc, depthStencilState.GetAddressOf());
	}
	// ラスタライザー（ソリッド・背面カリング）
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&desc, rasterizerState.GetAddressOf());
	}
}

void PhysicsDebugRenderer::Render(
	ID3D11DeviceContext* dc,
	physx::PxScene* pxScene, 
	physx::PxControllerManager* pxControllerManager,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection,
	const DirectX::XMFLOAT4& lightDirection)
{
	// PxShapeをDrawする内部ラムダ
	physx::PxShape* pxShapes[128] = { nullptr };

	auto drawShape = [&](
		physx::PxShape* pxShape,
		const physx::PxTransform& pxShapeTransform,
		float contactOffset,
		bool sleeping)
		{
			const physx::PxGeometry& pxGeometry = pxShape->getGeometry();
			const physx::PxMat44 pxShapeMat(pxShapeTransform);

			// PhysXの行列をDirectXの行列に変換
			DirectX::XMFLOAT4X4 shapeTransform = {
				pxShapeMat.column0.x, pxShapeMat.column0.y, pxShapeMat.column0.z, pxShapeMat.column0.w,
				pxShapeMat.column1.x, pxShapeMat.column1.y, pxShapeMat.column1.z, pxShapeMat.column1.w,
				pxShapeMat.column2.x, pxShapeMat.column2.y, pxShapeMat.column2.z, pxShapeMat.column2.w,
				pxShapeMat.column3.x, pxShapeMat.column3.y, pxShapeMat.column3.z, pxShapeMat.column3.w,
			};
			DirectX::XMFLOAT3 shapePosition = {
				pxShapeMat.column3.x, pxShapeMat.column3.y, pxShapeMat.column3.z
			};

			// スリープ中は暗く表示
			DirectX::XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 0.3f };
			if (sleeping)
			{
				const float dark = 0.25f;
				color.x *= dark;
				color.y *= dark;
				color.z *= dark;
			}

			switch (pxGeometry.getType())
			{
			case physx::PxGeometryType::eSPHERE:
			{
				const auto& g = static_cast<const physx::PxSphereGeometry&>(pxGeometry);
				shapeRenderer.DrawSphere(shapePosition, g.radius + contactOffset, color);
				break;
			}
			case physx::PxGeometryType::eCAPSULE:
			{
				// PhysXのカプセルはX軸向き → Z回転でY軸向きに変換
				const auto& g = static_cast<const physx::PxCapsuleGeometry&>(pxGeometry);
				DirectX::XMMATRIX offset = DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2);
				DirectX::XMMATRIX base = DirectX::XMLoadFloat4x4(&shapeTransform);
				DirectX::XMStoreFloat4x4(&shapeTransform, offset * base);
				shapeRenderer.DrawCapsule(
					shapeTransform,
					g.radius + contactOffset,
					g.halfHeight,
					color);
				break;
			}
			case physx::PxGeometryType::eBOX:
			{
				const auto& g = static_cast<const physx::PxBoxGeometry&>(pxGeometry);
				shapeRenderer.DrawBox(
					shapeTransform,
					{ g.halfExtents.x * 2.0f + contactOffset,
					  g.halfExtents.y * 2.0f + contactOffset,
					  g.halfExtents.z * 2.0f + contactOffset },
					color);
				break;
			}
			case physx::PxGeometryType::eTRIANGLEMESH:
			{
				const auto& g = static_cast<const physx::PxTriangleMeshGeometry&>(pxGeometry);

				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
					g.scale.scale.x, g.scale.scale.y, g.scale.scale.z);
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
					DirectX::XMVectorSet(
						g.scale.rotation.x, g.scale.rotation.y,
						g.scale.rotation.z, g.scale.rotation.w));
				DirectX::XMMATRIX ST = S * R * DirectX::XMLoadFloat4x4(&shapeTransform);

				const TriangleMeshCache* cache =
					GetOrCreateTriangleMeshCache(device, g, ST, color);
				if (!cache) break;

				// 描画リストに積む
				triangleMeshDrawList.push_back(cache);				
				break;
			}
			default:
				break;
			}
		};

	// Actorを描画するラムダ
	auto drawActor = [&](physx::PxRigidActor* pxActor, float contactOffset)
		{
			const physx::PxU32 numShapes = pxActor->getNbShapes();
			pxActor->getShapes(pxShapes, numShapes);

			physx::PxRigidDynamic* dynamic = pxActor->is<physx::PxRigidDynamic>();
			bool sleeping = dynamic ? dynamic->isSleeping() : false;

			for (physx::PxU32 i = 0; i < numShapes; ++i)
			{
				drawShape(pxShapes[i],
					physx::PxShapeExt::getGlobalPose(*pxShapes[i], *pxActor),
					contactOffset,
					sleeping);
			}
		};

	// Controllerが内部で持つActorを、あらかじめ集めておく(Static/Dynamicループで二重描画しないため)
	std::vector<physx::PxRigidActor*> controllerActors;
	if (pxControllerManager)
	{
		physx::PxU32 numControllers = pxControllerManager->getNbControllers();
		for (physx::PxU32 i = 0; i < numControllers; ++i)
		{
			physx::PxController* ctrl = pxControllerManager->getController(i);
			if (physx::PxRigidActor* a = ctrl->getActor())
			{
				controllerActors.push_back(a);
			}
		}
	}

	// Static / Dynamic Actor(Controllerのactorは除外する)
	{
		physx::PxActorTypeFlags flags =
			physx::PxActorTypeFlag::eRIGID_STATIC |
			physx::PxActorTypeFlag::eRIGID_DYNAMIC;
		physx::PxU32 numActors = pxScene->getNbActors(flags);
		if (numActors > 0)
		{
			std::vector<physx::PxRigidActor*> actors(numActors);
			pxScene->getActors(flags,
				reinterpret_cast<physx::PxActor**>(actors.data()), numActors);
			for (auto* actor : actors)
			{
				bool isControllerActor = std::find(controllerActors.begin(), controllerActors.end(), actor) != controllerActors.end();
				if (isControllerActor) continue;

				drawActor(actor, 0.0f);
			}
		}
	}


	// Controller
	if (pxControllerManager)
	{
		physx::PxU32 numControllers = pxControllerManager->getNbControllers();
		for (physx::PxU32 i = 0; i < numControllers; ++i)
		{
			physx::PxController* ctrl = pxControllerManager->getController(i);
			if (ctrl->getType() != physx::PxControllerShapeType::eCAPSULE) continue;

			physx::PxCapsuleController* capsuleCtrl = static_cast<physx::PxCapsuleController*>(ctrl);
			const float radius = capsuleCtrl->getRadius();
			const float halfHeight = capsuleCtrl->getHeight() * 0.5f;
			const float contactOffset = ctrl->getContactOffset();

			// 足元から中心へ
			const physx::PxExtendedVec3 footPos = ctrl->getFootPosition();
			const float centerY = static_cast<float>(footPos.y) + halfHeight + radius + contactOffset;

			// 最初から Y 軸向きのカプセルとして描く
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(
				static_cast<float>(footPos.x), centerY, static_cast<float>(footPos.z)));

			// 外側：壁や床に触れて止まる大きさ
			const DirectX::XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 0.3f };
			shapeRenderer.DrawCapsule(world, radius + contactOffset, halfHeight, color);
		}
	}

	// レイキャストデバッグ表示
	{
		std::vector<DebugRay> debugRays = PhysicsSystem::FetchDebugRays();
		for (const DebugRay& ray : debugRays)
		{
			primitiveRenderer.AddVertex(ray.start, ray.color);
			primitiveRenderer.AddVertex(ray.end, ray.color);
		}
	}

	// レンダーステート設定
	float blendFactor[4] = { 0, 0, 0, 0 };
	dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);
	dc->RSSetState(rasterizerState.Get());

	shapeRenderer.Render(dc, view, projection,
		{ lightDirection.x, lightDirection.y, lightDirection.z });

	// PrimitiveRendererのシェーダー・レイアウトをセットしてからDraw
	primitiveRenderer.Render(dc, view, projection);

	// TriangleMeshキャッシュをDraw（primitiveRenderer.Renderでシェーダーがセット済み）
	if (!triangleMeshDrawList.empty())
	{
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		for (const TriangleMeshCache* cache : triangleMeshDrawList)
		{
			UINT stride = sizeof(PrimitiveRenderer::Vertex);
			UINT offset = 0;
			dc->IASetVertexBuffers(0, 1,
				cache->vertexBuffer.GetAddressOf(), &stride, &offset);
			dc->Draw(cache->vertexCount, 0);
		}
		triangleMeshDrawList.clear();
	}
}

// キャッシュの取得
const PhysicsDebugRenderer::TriangleMeshCache*
PhysicsDebugRenderer::GetOrCreateTriangleMeshCache(
	ID3D11Device* device,
	const physx::PxTriangleMeshGeometry& geom,
	const DirectX::XMMATRIX& shapeTransform,
	const DirectX::XMFLOAT4& color)
{
	const physx::PxTriangleMesh* pxMesh = geom.triangleMesh;

	// すでにキャッシュがあればそのまま返す
	auto it = triangleMeshCache.find(pxMesh);
	if (it != triangleMeshCache.end())
	{
		return &it->second;
	}

	// キャッシュがないので新規作成
	const physx::PxVec3* verts = pxMesh->getVertices();
	const void* idxData = pxMesh->getTriangles();
	const physx::PxU32* idx32 = static_cast<const physx::PxU32*>(idxData);
	const physx::PxU16* idx16 = static_cast<const physx::PxU16*>(idxData);
	bool has16 = static_cast<bool>(
		pxMesh->getTriangleMeshFlags() & physx::PxTriangleMeshFlag::e16_BIT_INDICES);

	physx::PxU32 numTri = pxMesh->getNbTriangles();

	// ワールド変換済みの頂点データをCPUで作成（ラインリスト: 三角形1枚=6頂点）
	struct CacheVertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};
	std::vector<CacheVertex> cpuVerts;
	cpuVerts.reserve(numTri * 6);

	for (physx::PxU32 t = 0; t < numTri; ++t)
	{
		physx::PxU32 i0, i1, i2;
		if (has16) { i0 = *idx16++; i1 = *idx16++; i2 = *idx16++; }
		else { i0 = *idx32++; i1 = *idx32++; i2 = *idx32++; }

		auto toFloat3 = [&](const physx::PxVec3& v) -> DirectX::XMFLOAT3
			{
				DirectX::XMFLOAT3 out;
				DirectX::XMStoreFloat3(&out,
					DirectX::XMVector3Transform(
						DirectX::XMVectorSet(v.x, v.y, v.z, 0), shapeTransform));
				return out;
			};

		DirectX::XMFLOAT3 v0 = toFloat3(verts[i0]);
		DirectX::XMFLOAT3 v1 = toFloat3(verts[i1]);
		DirectX::XMFLOAT3 v2 = toFloat3(verts[i2]);

		cpuVerts.push_back({ v0, color });
		cpuVerts.push_back({ v1, color });
		cpuVerts.push_back({ v1, color });
		cpuVerts.push_back({ v2, color });
		cpuVerts.push_back({ v2, color });
		cpuVerts.push_back({ v0, color });
	}

	if (cpuVerts.empty()) return nullptr;

	// GPUバッファを作成
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(CacheVertex) * cpuVerts.size());
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = cpuVerts.data();

	TriangleMeshCache cache;
	HRESULT hr = device->CreateBuffer(&desc, &init, cache.vertexBuffer.GetAddressOf());
	if (FAILED(hr)) return nullptr;

	cache.vertexCount = static_cast<UINT>(cpuVerts.size());
	triangleMeshCache[pxMesh] = std::move(cache);

	return &triangleMeshCache[pxMesh];
}