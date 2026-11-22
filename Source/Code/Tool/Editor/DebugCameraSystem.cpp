#include "DebugCameraSystem.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

void DebugCameraSystem::Update(entt::registry& registry, float elapsedTime, bool viewportHovered)
{
    auto view = registry.view<FreeCameraComponent>();
    view.each([&](auto entity, FreeCameraComponent& free)
        {
            UpdateFreeCamera(free, elapsedTime, viewportHovered);
        });
}

void DebugCameraSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
    auto view = registry.view<FreeCameraComponent>();
    for (auto entity : view)
    {
        auto& free = view.get<FreeCameraComponent>(entity);
        camera.SetLookAt(
            free.base.currentEye,
            free.base.currentTarget,
            { 0.0f, 1.0f, 0.0f }
        );
        // 最初の1つだけ反映
        break;
    }
}

void DebugCameraSystem::UpdateFreeCamera(FreeCameraComponent& free, float elapsedTime, bool viewportHovered)
{
    // Gameビューポート上にマウスが無い時は処理しない
    if (!viewportHovered) return;

    // 右クリック中だけ操作を受け付ける
    if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000))
    {
        free.firstMouse = true;
        return;
    }

    // ホイールで速度変更
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f)
    {
        free.cameraSpeed += wheel * 2.0f;
        free.cameraSpeed = std::clamp(free.cameraSpeed, 0.1f, 100.0f);
    }

    float move = free.cameraSpeed * elapsedTime;

    // currentEye / currentTarget から前後左右を計算
    DirectX::XMVECTOR eyeV = DirectX::XMLoadFloat3(&free.base.currentEye);
    DirectX::XMVECTOR targetV = DirectX::XMLoadFloat3(&free.base.currentTarget);
    DirectX::XMVECTOR frontV = DirectX::XMVector3Normalize(
        DirectX::XMVectorSubtract(targetV, eyeV));
    DirectX::XMVECTOR upV = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR rightV = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(frontV, upV));

    DirectX::XMFLOAT3 front, right;
    DirectX::XMStoreFloat3(&front, frontV);
    DirectX::XMStoreFloat3(&right, rightV);

    DirectX::XMFLOAT3 eye = free.base.currentEye;
    DirectX::XMFLOAT3 target = free.base.currentTarget;

    // WASD 前後左右移動
    if (GetAsyncKeyState('W') & 0x8000)
    {
        eye.x += front.x * move; eye.y += front.y * move; eye.z += front.z * move;
        target.x += front.x * move; target.y += front.y * move; target.z += front.z * move;
    }
    if (GetAsyncKeyState('S') & 0x8000)
    {
        eye.x -= front.x * move; eye.y -= front.y * move; eye.z -= front.z * move;
        target.x -= front.x * move; target.y -= front.y * move; target.z -= front.z * move;
    }
    if (GetAsyncKeyState('D') & 0x8000)
    {
        eye.x -= right.x * move; eye.y -= right.y * move; eye.z -= right.z * move;
        target.x -= right.x * move; target.y -= right.y * move; target.z -= right.z * move;
    }
    if (GetAsyncKeyState('A') & 0x8000)
    {
        eye.x += right.x * move; eye.y += right.y * move; eye.z += right.z * move;
        target.x += right.x * move; target.y += right.y * move; target.z += right.z * move;
    }

    // QE 上下移動
    if (GetAsyncKeyState('E') & 0x8000) { eye.y += move; target.y += move; }
    if (GetAsyncKeyState('Q') & 0x8000) { eye.y -= move; target.y -= move; }

    // マウスで視点回転
    POINT mousePos;
    GetCursorPos(&mousePos);

    if (free.firstMouse)
    {
        free.lastMousePos = mousePos;
        free.firstMouse = false;

        // 現在の向きから yaw / pitch を復元
        DirectX::XMFLOAT3 dir =
        {
            target.x - eye.x,
            target.y - eye.y,
            target.z - eye.z
        };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.0f)
        {
            dir.x /= len; dir.y /= len; dir.z /= len;
        }
        free.pitch = asinf(dir.y);
        free.yaw = atan2f(dir.x, dir.z);
    }

    float dx = static_cast<float>(mousePos.x - free.lastMousePos.x) * 0.005f;
    float dy = static_cast<float>(mousePos.y - free.lastMousePos.y) * 0.005f;

    free.yaw += dx;
    free.pitch -= dy;
    free.pitch = std::clamp(free.pitch,
        -DirectX::XM_PIDIV2 + 0.1f,
        DirectX::XM_PIDIV2 - 0.1f);

    free.lastMousePos = mousePos;

    // 前方向ベクトルから target を更新
    DirectX::XMFLOAT3 direction;
    direction.x = cosf(free.pitch) * sinf(free.yaw);
    direction.y = sinf(free.pitch);
    direction.z = cosf(free.pitch) * cosf(free.yaw);

    target.x = eye.x + direction.x;
    target.y = eye.y + direction.y;
    target.z = eye.z + direction.z;

    free.base.currentEye = eye;
    free.base.currentTarget = target;
}