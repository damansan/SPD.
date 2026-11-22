#include "PhysicsSystem.h"
#include "Engine/Core/PhysicsDestroyQueue.h"
#include "Engine/Core/PhysicsLayer.h"

#include "Engine/ECS/Components/PhysicsComponents.h"
#include "Engine/ECS/Components/StaticMeshComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/Resource/Model.h"

using namespace physx;

std::vector<DebugRay> PhysicsSystem::debugRays;

// TransformComponentの回転からPxQuatを作る
static PxQuat ToPxQuat(const DirectX::XMFLOAT3& rotation)
{
	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(
		rotation.x, rotation.y, rotation.z);
	DirectX::XMFLOAT4 qf;
	DirectX::XMStoreFloat4(&qf, q);
	return PxQuat(qf.x, qf.y, qf.z, qf.w);
}

namespace
{

	// 横に当たった面を1つ覚えておく
	class SideHitCollector : public PxUserControllerHitReport
	{
	public:
		void Reset(const PxVec3& moveDisp)
		{
			hasHit = false;
			bestNormalY = 1.0f;
			normal = PxVec3(0.0f, 0.0f, 0.0f);
			intendedDir = PxVec3(moveDisp.x, 0.0f, moveDisp.z); // 水平だけ

			hasGround = false;
			bestGroundY = -1.0f;
			groundNormal = PxVec3(0.0f, 1.0f, 0.0f);
		}

		void onShapeHit(const PxControllerShapeHit& hit) override
		{
			// 床：一番「上向き」の面を残す(裏返った面に当たって下を向いていたら、上向きに直す)
			PxVec3 upNormal = hit.worldNormal;
			if (upNormal.y < 0.0f)
			{
				upNormal = -upNormal;
			}
			if (!hasGround || upNormal.y > bestGroundY)
			{
				hasGround = true;
				bestGroundY = upNormal.y;
				groundNormal = upNormal;
			}

			// 保険：裏返った面や板に裏から当たっても、法線がプレイヤー側を向くようにする
			PxVec3 hitNormal = hit.worldNormal;
			if (hitNormal.dot(intendedDir) > 0.0f)
			{
				hitNormal = -hitNormal;
			}

			// 一番「横向き」の面(法線の y が 0 に近い)を残す
			const float normalY = fabsf(hitNormal.y);
			if (!hasHit || normalY < bestNormalY)
			{
				hasHit = true;
				bestNormalY = normalY;
				normal = hitNormal;
			}
		}

		void onControllerHit(const PxControllersHit&) override {}      // CCT 同士
		void onObstacleHit(const PxControllerObstacleHit&) override {} // 使っていない

		bool   hasHit = false;
		float  bestNormalY = 1.0f;
		PxVec3 normal = PxVec3(0.0f, 0.0f, 0.0f);
		PxVec3 intendedDir = PxVec3(0.0f, 0.0f, 0.0f); // このフレームに動かしたかった水平の向き

		bool   hasGround = false;
		float  bestGroundY = -1.0f;
		PxVec3 groundNormal = PxVec3(0.0f, 1.0f, 0.0f);
	};

	// すべての CCT で1つを使い回す
	SideHitCollector sideHitCollector;

	// StaticBodyComponentが外れる／Entityが破棄される直前に、Actorを破棄予約する
	void OnStaticBodyDestroyed(entt::registry& registry, entt::entity entity)
	{
		if (auto* body = registry.try_get<StaticBodyComponent>(entity))
		{
			PhysicsDestroyQueue::Push(body->actor);
			body->actor = nullptr; // 二重予約の保険
		}
	}

	// CharacterControllerComponentが外れる／Entityが破棄される直前に、Controllerを破棄予約する
	void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity)
	{
		if (auto* ctrl = registry.try_get<CharacterControllerComponent>(entity))
		{
			PhysicsDestroyQueue::Push(ctrl->controller);
			ctrl->controller = nullptr; // 二重予約の保険
		}
	}

	// 足元 foot に「全体の高さ height・半径 radius」のカプセルを置いたら、Static/Dynamic に重なるか
	bool OverlapsCapsuleAt(PxScene* scene, const PxExtendedVec3& foot, float contactOffset, float radius, float height)
	{
		const float halfCylinder = CharacterControllerComponent::CylinderHeight(height, radius) * 0.5f;
		const float centerY = static_cast<float>(foot.y) + contactOffset + radius + halfCylinder;

		// PhysX のカプセルは X 軸向き → Z 軸まわりに 90° 回して Y 軸向きにする
		const PxTransform pose(
			PxVec3(static_cast<float>(foot.x), centerY, static_cast<float>(foot.z)),
			PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
		const PxCapsuleGeometry geometry(radius, halfCylinder);

		// 自分(Character レイヤー)は除いて、Static と Dynamic だけを見る
		PhysicsQueryFilter filter(PhysicsLayer::Static | PhysicsLayer::Dynamic);
		PxQueryFilterData filterData;
		filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT;

		PxOverlapBuffer overlapHit;
		return scene->overlap(geometry, pose, overlapHit, filterData, &filter);
	}
}

// 更新
void PhysicsSystem::Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime)
{
	SyncBodyTypes(registry);
	ProcessDestroyQueue();
	ProcessStaticBodies(registry, physX, pxScene);
	// ProcessDynamicBodies(registry, physX, pxScene);
	ProcessCharacterControllers(registry, physX, controllerManager, elapsedTime);

}

// 剛体コンポーネントが外れる/エンティティが破壊される前にPhysXの実体を削除する
void PhysicsSystem::ConnectDestroyListeners(entt::registry& registry)
{
	registry.on_destroy<StaticBodyComponent>().connect<&OnStaticBodyDestroyed>();
	registry.on_destroy<CharacterControllerComponent>().connect<&OnCharacterControllerDestroyed>();
}

// レイヤーにレイキャスト
RaycastHitResult PhysicsSystem::Raycast(physx::PxScene* pxScene, const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float maxDistance, physx::PxU32 wantedLayers)
{
	RaycastHitResult result;

	// 開始位置をPxVec3へ変換
	PxVec3 pxOrigin(origin.x, origin.y, origin.z);

	// 方向を正規化してPxVec3へ変換
	DirectX::XMVECTOR dirVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));
	DirectX::XMFLOAT3 normalizedDir;
	DirectX::XMStoreFloat3(&normalizedDir, dirVec);
	PxVec3 pxDirection(normalizedDir.x, normalizedDir.y, normalizedDir.z);

	// 欲しいレイヤーだけを通す
	PhysicsQueryFilter filter(wantedLayers);

	// 欲しい剛体で絞り込む
	PxQueryFilterData filterData;
	filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

	// 結果を受け取る
	PxRaycastBuffer hitBuffer;

	// レイを飛ばす
	bool status = pxScene->raycast(
		pxOrigin, pxDirection, maxDistance,
		hitBuffer,
		PxHitFlag::eDEFAULT,
		filterData,
		&filter);

	// 当たった結果を取り出す
	if (status && hitBuffer.hasBlock)
	{
		result.hit = true;
		result.distance = hitBuffer.block.distance;
		result.position = { hitBuffer.block.position.x, hitBuffer.block.position.y, hitBuffer.block.position.z };
		result.normal = { hitBuffer.block.normal.x,   hitBuffer.block.normal.y,   hitBuffer.block.normal.z };
	}

	// レイを記録
	DebugRay debugRay;
	debugRay.start = origin;
	float drawDistance = result.hit ? result.distance : maxDistance;
	debugRay.end = {
		origin.x + normalizedDir.x * drawDistance,
		origin.y + normalizedDir.y * drawDistance,
		origin.z + normalizedDir.z * drawDistance
	};
	debugRay.color = result.hit
		? DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)  // 当たれば赤
		: DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f); // 当たらなければ青
	debugRays.push_back(debugRay);

	return result;
}

std::vector<DebugRay> PhysicsSystem::FetchDebugRays()
{
	std::vector<DebugRay> result = std::move(debugRays);
	debugRays.clear();
	return result;
}

// PhysicsBodyTypeComponent::type に合わせて、剛体コンポーネントを付け外しする
void PhysicsSystem::SyncBodyTypes(entt::registry& registry)
{
	auto view = registry.view<PhysicsBodyTypeComponent>();

	for (auto entity : view)
	{
		PhysicsBodyType type = view.get<PhysicsBodyTypeComponent>(entity).type;

		// Static
		if (type == PhysicsBodyType::Static)
		{
			if (!registry.all_of<StaticBodyComponent>(entity))
			{
				registry.emplace<StaticBodyComponent>(entity);
			}
		}
		else if (registry.all_of<StaticBodyComponent>(entity))
		{
			// 実体のActorの破棄予約は、on_destroyの通知で行われる
			registry.remove<StaticBodyComponent>(entity);
		}

		// Character（MovementComponentとセット）
		if (type == PhysicsBodyType::Character)
		{
			if (!registry.all_of<CharacterControllerComponent>(entity))
			{
				registry.emplace<CharacterControllerComponent>(entity);
			}
			if (!registry.all_of<MovementComponent>(entity))
			{
				registry.emplace<MovementComponent>(entity);
			}
			if (!registry.all_of<CharacterContactComponent>(entity))
			{
				registry.emplace<CharacterContactComponent>(entity);
			}
		}
		else if (registry.all_of<CharacterControllerComponent>(entity))
		{
			// Controllerの破棄予約は、on_destroyの通知で行われる
			registry.remove<CharacterControllerComponent>(entity);

			if (registry.all_of<MovementComponent>(entity))
			{
				registry.remove<MovementComponent>(entity);
			}
			if (registry.all_of<CharacterContactComponent>(entity))
			{
				registry.remove<CharacterContactComponent>(entity);
			}
		}
	}
}

// 削除予約されたActorを解放する
void PhysicsSystem::ProcessDestroyQueue()
{
	auto& destroyQueue = PhysicsDestroyQueue::GetQueue();
	for (PxActor* actor : destroyQueue)
	{
		if (!actor) continue;
		if (actor->getScene())
		{
			actor->getScene()->removeActor(*actor);
		}
		actor->release();
	}
	destroyQueue.clear();

	// Controllerの解放
	auto& controllerQueue = PhysicsDestroyQueue::GetControllerQueue();
	for (PxController* controller : controllerQueue)
	{
		if (!controller) continue;
		controller->release();
	}
	controllerQueue.clear();
}

// 未生成のStaticBodyComponent生成
void PhysicsSystem::ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene)
{
	auto view = registry.view<StaticBodyComponent, StaticMeshComponent, TransformComponent>();

	for (auto entity : view)
	{
		auto& body = view.get<StaticBodyComponent>(entity);

		// 既に生成済みならスキップ
		if (body.actor != nullptr) continue;

		auto& staticMesh = view.get<StaticMeshComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		// モデルが未ロードならスキップ
		if (!staticMesh.model) continue;

		// Actorの位置・回転を設定
		PxTransform pxTransform(
			PxVec3(transform.position.x, transform.position.y, transform.position.z),
			ToPxQuat(transform.rotation)
		);

		PxRigidStatic* actor = physX.GetPhysics()->createRigidStatic(pxTransform);

		// モデル内の各Meshごとにシェイプを追加
		int shapeCount = 0;
		for (const auto& mesh : staticMesh.model->GetMeshes())
		{
			if (mesh.vertices.empty() || mesh.indices.empty()) continue;

			PxTriangleMesh* triMesh = physX.CreateTriangleMesh(
				mesh.vertices.data(),
				static_cast<uint32_t>(mesh.vertices.size()),
				sizeof(Model::Vertex),
				mesh.indices.data(),
				static_cast<uint32_t>(mesh.indices.size() / 3)
			);

			if (!triMesh) continue; // メッシュ生成失敗

			PxMeshScale meshScale(
				PxVec3(transform.scale.x, transform.scale.y, transform.scale.z)
			);
			PxTriangleMeshGeometry geometry(triMesh, meshScale);

			PxShape* shape = PxRigidActorExt::createExclusiveShape(
				*actor, geometry, *physX.GetDefaultMaterial()
			);

			triMesh->release();

			if (shape)
			{
				// Staticレイヤーを付与
				shape->setQueryFilterData(PhysicsLayer::MakeFilterData(PhysicsLayer::Static));
				shapeCount++;
			}
		}

		// シェイプが1つも作れなかった場合はActorごと破棄
		if (shapeCount == 0)
		{
			actor->release();
			continue;
		}

		pxScene->addActor(*actor);
		body.actor = actor;
	}
}

// 未生成のCharacterControllerComponent生成+CharacterController移動処理
void PhysicsSystem::ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime)
{
	auto view = registry.view<CharacterControllerComponent, TransformComponent>();

	for (auto entity : view)
	{
		auto& ctrl = view.get<CharacterControllerComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		// 未生成ならController本体を生成する
		if (ctrl.controller == nullptr)
		{
			PxCapsuleControllerDesc desc;
			desc.radius = ctrl.radius;                                                        // カプセル半径
			desc.height = CharacterControllerComponent::CylinderHeight(ctrl.height, ctrl.radius); // 円柱部分の長さ
			desc.position = PxExtendedVec3(transform.position.x, transform.position.y, transform.position.z); // 生成位置
			desc.material = physX.GetDefaultMaterial();  // 物理マテリアル
			desc.upDirection = PxVec3(0.0f, 1.0f, 0.0f); // Y軸+を上方向とする

			desc.stepOffset = 0.2f; // 自動で登る段差の上限
			//desc.slopeLimit = cosf(physx::PxDegToRad(45.0f)); // 歩行可能な坂の角度
			//desc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;  // 段差の登りをより厳密に制限
			desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
			desc.reportCallback = &sideHitCollector; // 当たった面の報告先
			//desc.contactOffset = 0.01f; // 接触検出のマージンを狭める(検出精度UP)
			
			
			if (!desc.isValid()) continue; // パラメータ不正なら生成しない

			ctrl.controller = controllerManager->createController(desc);
			ctrl.appliedRadius = ctrl.radius;
			ctrl.appliedHeight = ctrl.height;

			ctrl.controller->setFootPosition(PxExtendedVec3(transform.position.x, transform.position.y, transform.position.z));

			// Characterレイヤー付与
			if (PxRigidDynamic* controllerActor = ctrl.controller->getActor())
			{
				PxShape* controllerShape = nullptr;
				if (controllerActor->getShapes(&controllerShape, 1) == 1 && controllerShape)
				{
					controllerShape->setQueryFilterData(PhysicsLayer::MakeFilterData(PhysicsLayer::Character));
				}
			}
			// 生成直後のフレームは移動させない（次フレームから移動を反映する）
			continue;
		}

		// MovementComponentが無いEntityは移動させない（その場で待機）
		auto* movement = registry.try_get<MovementComponent>(entity);

		// 当たり判定の高さと半径
		if (movement)
		{
			const float wantedHeight = ctrl.height * movement->heightScale;
			float       wantedRadius = movement->radiusOverride > 0.0f ? movement->radiusOverride : ctrl.radius;
			movement->heightScale = 1.0f;
			movement->radiusOverride = 0.0f;

			const PxExtendedVec3 foot = ctrl.controller->getFootPosition();

			// 太くする時だけ：周りに空間がなければ、今の太さのまま(空間ができたフレームで太くなる)
			if (wantedRadius > ctrl.appliedRadius + 1e-4f &&
				OverlapsCapsuleAt(ctrl.controller->getScene(), foot, ctrl.controller->getContactOffset(), wantedRadius, wantedHeight))
			{
				wantedRadius = ctrl.appliedRadius;
			}

			// 変わった時だけ設定し直す
			const bool heightChanged = fabsf(wantedHeight - ctrl.appliedHeight) > 1e-4f;
			const bool radiusChanged = fabsf(wantedRadius - ctrl.appliedRadius) > 1e-4f;
			if (heightChanged || radiusChanged)
			{
				auto* capsule = static_cast<PxCapsuleController*>(ctrl.controller);
				capsule->setRadius(wantedRadius);
				capsule->resize(CharacterControllerComponent::CylinderHeight(wantedHeight, wantedRadius));
				capsule->setFootPosition(foot); // 半径を変えると中心基準で伸び縮みするので、足元を元の位置に戻す

				ctrl.appliedRadius = wantedRadius;
				ctrl.appliedHeight = wantedHeight;
			}
		}

		PxVec3 disp(0.0f, 0.0f, 0.0f);
		if (movement)
		{
			disp = PxVec3(
				movement->velocity.x * elapsedTime,
				movement->velocity.y * elapsedTime,
				movement->velocity.z * elapsedTime
			);

			// 下り坂で浮かないように：前のフレームで接地していて、上へ動いていない時だけ
			// 水平の移動量に合わせて、坂に沿う分だけ下へずらす(上り坂・平地では何もしない)
			const auto* contact = registry.try_get<CharacterContactComponent>(entity);
			if (contact && movement->isGrounded && movement->velocity.y <= 0.0f)
			{
				const DirectX::XMFLOAT3& n = contact->groundNormal;

				// 歩ける程度の坂だけ(急すぎる面は PhysX に任せて滑らせる)
				constexpr float kMinFollowNormalY = 0.7f;
				if (n.y >= kMinFollowNormalY)
				{
					const float followY = -(n.x * disp.x + n.z * disp.z) / n.y;
					if (followY < 0.0f)
					{
						disp.y += followY;
					}
				}
			}
		}

		// 移動を実行（壁・段差・地面との衝突解決はPhysXが行う）
		sideHitCollector.Reset(disp);
		PxControllerFilters filters;
		PxControllerCollisionFlags collisionFlags = ctrl.controller->move(disp, 0.001f, elapsedTime, filters);

		// 結果の位置をTransformへ反映（足元座標）
		PxExtendedVec3 foot = ctrl.controller->getFootPosition();
		transform.position.x = static_cast<float>(foot.x);
		transform.position.y = static_cast<float>(foot.y);
		transform.position.z = static_cast<float>(foot.z);

		// 接触の結果を反映
		if (auto* contact = registry.try_get<CharacterContactComponent>(entity))
		{
			const bool sides = (collisionFlags & PxControllerCollisionFlag::eCOLLISION_SIDES);
			contact->hitSide = sides && sideHitCollector.hasHit;
			contact->sideNormal = contact->hitSide
				? DirectX::XMFLOAT3{ sideHitCollector.normal.x, sideHitCollector.normal.y, sideHitCollector.normal.z }
			: DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };

			// 床：下に当たった時だけ法線を入れる(当たっていなければ真上)
			const bool down = (collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN);
			contact->hitGround = false;
			contact->groundNormal = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };
			if (down && sideHitCollector.hasGround)
			{
				contact->hitGround = true;
				contact->groundNormal = DirectX::XMFLOAT3{ sideHitCollector.groundNormal.x, sideHitCollector.groundNormal.y, sideHitCollector.groundNormal.z };
			}

			// 立てるか：今のカプセルが立った時より低い時だけ調べる(立っている時は常に true)
			contact->canStand = true;
			if (ctrl.appliedHeight < ctrl.height - 1e-4f)
			{
				// 立った時のカプセル(今の半径・元の高さ)が、何かに重なるか
				contact->canStand = !OverlapsCapsuleAt(
					ctrl.controller->getScene(), ctrl.controller->getFootPosition(),
					ctrl.controller->getContactOffset(), ctrl.appliedRadius, ctrl.height);
			}
		}

		// 接地時に常にかけておく、地面に押し付ける力の大きさ
		constexpr float kGroundStickSpeed = 2.0f;

		// 接地判定をMovementComponentへ反映
		if (movement)
		{
			// このフレームで使った水平速度の大きさ
			movement->planarSpeed = sqrtf(
				movement->velocity.x * movement->velocity.x +
				movement->velocity.z * movement->velocity.z);

			bool grounded = (collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN);
			movement->isGrounded = grounded;

			// 地面へ押し付ける力をかける
			if (grounded && movement->velocity.y <= 0.0f)
			{
				movement->velocity.y = -kGroundStickSpeed;
			}
		}
	}
}
