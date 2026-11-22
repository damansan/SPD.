#pragma once
#include <windows.h>
#include "BaseCameraComponent.h"

// デバッグ用フリーカメラコンポーネント
struct FreeCameraComponent
{
    BaseCameraComponent base;  // 共通データ（eye, target）

    float cameraSpeed = 5.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool  firstMouse = true;
    POINT lastMousePos = {};
};