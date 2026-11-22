#pragma once
#include <PxPhysicsAPI.h>


// カテゴライズ
namespace PhysicsLayer
{
    constexpr physx::PxU32 Static    = 1 << 0; // 静的剛体
    constexpr physx::PxU32 Dynamic   = 1 << 1; // 動的剛体
    constexpr physx::PxU32 Character = 1 << 2; // キャラクターコントローラー

    // PxFilterData作成
    inline physx::PxFilterData MakeFilterData(physx::PxU32 layer)
    {
        return physx::PxFilterData(layer, 0, 0, 0);
    }
}

// フィルタリング
// フィルタリング
class PhysicsQueryFilter : public physx::PxQueryFilterCallback
{
public:
    explicit PhysicsQueryFilter(physx::PxU32 wantedLayers) : wantedLayers(wantedLayers) 
    {
    }

    physx::PxQueryHitType::Enum preFilter(
        const physx::PxFilterData& filterData,
        const physx::PxShape* shape,
        const physx::PxRigidActor* actor,
        physx::PxHitFlags& queryFlags) override
    {
        if (shape->getQueryFilterData().word0 & wantedLayers)
        {
            return physx::PxQueryHitType::eBLOCK;
        }
        return physx::PxQueryHitType::eNONE;
    }

    physx::PxQueryHitType::Enum postFilter(
        const physx::PxFilterData&, const physx::PxQueryHit&,
        const physx::PxShape*, const physx::PxRigidActor*) override
    {
        return physx::PxQueryHitType::eBLOCK;
    }

private:
    physx::PxU32 wantedLayers;
};