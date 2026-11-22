#include "EngineComponents.h"
#include "Engine/Resource/ComponentRegistry.h"

#include "Engine/ECS/Components/CategoryComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MeshComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"
#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/FreeCameraComponent.h"
#include "Engine/ECS/Components/PhysicsComponents.h"
#include "Engine/ECS/Components/IKComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/CharacterContactComponent.h"

void EngineComponents::Register()
{
	REGISTER_COMPONENT_TYPE(CategoryComponent, "Category");
	REGISTER_COMPONENT_TYPE(TransformComponent, "Transform");
	REGISTER_COMPONENT_TYPE(MeshComponent, "Mesh");
	REGISTER_COMPONENT_TYPE(MaterialComponent, "Material");
	REGISTER_COMPONENT_TYPE(AnimatorComponent, "Animator");
	REGISTER_COMPONENT_TYPE(FreeCameraComponent, "FreeCamera");
	REGISTER_COMPONENT_TYPE(ControllerComponent, "Controller");
	REGISTER_COMPONENT_TYPE(IKComponent, "IK");
	REGISTER_COMPONENT_TYPE(SequencerComponent, "Sequencer");
	REGISTER_COMPONENT_TYPE(AnimationSpeedComponent, "Animation Speed");
	REGISTER_COMPONENT_TYPE(StateMachineComponent, "State Machine");

	// 物理：種別(PhysicsBodyType)を代表として登録し、
	// 外すときは種別ごとに付与された個別のコンポーネントもまとめて外す
	ComponentRegistry::Register({
		"PhysicsBodyTypeComponent",
		"Physics",
		// Add: PhysicsBodyTypeComponentのみ付与(種別未選択状態)
		[](entt::registry& reg, entt::entity e)
		{
			if (!reg.any_of<PhysicsBodyTypeComponent>(e))
				reg.emplace<PhysicsBodyTypeComponent>(e);
		},
		// Remove: 共通コンポーネントと紐づいた個別のコンポーネントを両方削除
		[](entt::registry& reg, entt::entity e)
		{
			// 実体(PxActor/PxController)の破棄予約は、on_destroyの通知で自動的に行われる
			if (reg.any_of<StaticBodyComponent>(e))
				reg.remove<StaticBodyComponent>(e);

			if (reg.any_of<CharacterControllerComponent>(e))
				reg.remove<CharacterControllerComponent>(e);

			if (reg.any_of<MovementComponent>(e))
				reg.remove<MovementComponent>(e);

			if (reg.any_of<CharacterContactComponent>(e))
				reg.remove<CharacterContactComponent>(e);

			reg.remove<PhysicsBodyTypeComponent>(e);
		},
		// Has
		[](entt::registry& reg, entt::entity e) { return reg.any_of<PhysicsBodyTypeComponent>(e); },
		// GetMeta
		[](entt::registry& r, entt::entity e) -> entt::meta_any
		{
			auto* c = r.try_get<PhysicsBodyTypeComponent>(e);
			if (!c) return entt::meta_any{};
			return entt::forward_as_meta(*c);
		}
		});
}