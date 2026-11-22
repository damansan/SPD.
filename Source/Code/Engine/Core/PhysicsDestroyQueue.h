#pragma once
#include <vector>
#include <PxPhysicsAPI.h>

// 削除待ち物理Actorの一時保持キュー
namespace PhysicsDestroyQueue
{
    inline std::vector<physx::PxActor*>& GetQueue()
    {
        static std::vector<physx::PxActor*> queue;
        return queue;
    }

    // 解放予約する
    inline void Push(physx::PxActor* actor)
    {
        if (actor)
        {
            GetQueue().push_back(actor);
        }
    }


    // 削除待ちControllerの一時保持キュー
    inline std::vector<physx::PxController*>& GetControllerQueue()
    {
        static std::vector<physx::PxController*> queue;
        return queue;
    }

    // 解放予約する（Controller）
    inline void Push(physx::PxController* controller)
    {
        if (controller)
        {
            GetControllerQueue().push_back(controller);
        }
    }
}