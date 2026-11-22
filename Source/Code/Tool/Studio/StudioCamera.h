#pragma once
#include <DirectXMath.h>

class StudioCamera
{
public:
	// コンストラクタ
    StudioCamera();

    // 更新
    void Update(bool isWindowHovered, float elapsedTime);

    // 毎フレームウィンドウサイズを渡して比率を更新する
    void UpdateAspect(float width, float height);

    const DirectX::XMFLOAT4X4& GetView()       const { return view; }
    const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }
    const DirectX::XMFLOAT3& GetEye()          const { return eye; }

    bool IsControlling() const { return isControlling; }

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    DirectX::XMFLOAT3 GetFront() const;
    DirectX::XMFLOAT3 GetRight() const;

    DirectX::XMFLOAT4X4 view = {};
    DirectX::XMFLOAT4X4 projection = {};

    DirectX::XMFLOAT3 eye = { 0.0f, 3.0f, -4.0f };
    DirectX::XMFLOAT3 target = { 0.0f, 1.0f, 0.0f };

    float yaw = 0.0f;
    float pitch = 0.0f;
    float speed = 5.0f;

    // 現在のアスペクト比を保存
    float aspect = 16.0f / 9.0f;

    bool firstMouse = true;
    long lastMouseX = 0;
    long lastMouseY = 0;

    bool isControlling = false;
};