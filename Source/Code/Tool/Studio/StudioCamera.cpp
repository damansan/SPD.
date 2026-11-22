#include "StudioCamera.h"
#include <imgui.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>

// コンストラクタ
StudioCamera::StudioCamera()
{
	// 初期のeye/targetからyaw/pitchを計算
	DirectX::XMFLOAT3 dir =
	{
		target.x - eye.x,
		target.y - eye.y,
		target.z - eye.z
	};
	float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	if (len > 0.0f) { dir.x /= len; dir.y /= len; dir.z /= len; }

	yaw = atan2f(dir.x, dir.z);
	pitch = asinf(dir.y);

	// 初期射影行列
	UpdateProjectionMatrix();
	UpdateViewMatrix();
}

// 更新
void StudioCamera::Update(bool isWindowHovered, float elapsedTime)
{
	// 右クリックを離したら操作終了
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		isControlling = false;
		firstMouse = true;
		return;
	}
	// ウィンドウホバー中に右クリックを押し始めた瞬間だけ操作開始
	if (!isControlling)
	{
		if (isWindowHovered)
			isControlling = true;
		else
			return;
	}

	// if (!isControlling) return;

	// ホイールで速度変更
	float wheel = ImGui::GetIO().MouseWheel;
	if (wheel != 0.0f)
	{
		speed += wheel * 2.0f;
		speed = std::clamp(speed, 0.1f, 100.0f);
	}

	float move = speed * elapsedTime;
	DirectX::XMFLOAT3 front = GetFront();
	DirectX::XMFLOAT3 right = GetRight();

	auto MoveEyeTarget = [&](const DirectX::XMFLOAT3& axis, float amount)
		{
			eye.x += axis.x * amount; eye.y += axis.y * amount; eye.z += axis.z * amount;
			target.x += axis.x * amount; target.y += axis.y * amount; target.z += axis.z * amount;
		};

	if (GetAsyncKeyState('W') & 0x8000)  MoveEyeTarget(front, move);
	if (GetAsyncKeyState('S') & 0x8000)  MoveEyeTarget(front, -move);
	if (GetAsyncKeyState('D') & 0x8000)  MoveEyeTarget(right, move);
	if (GetAsyncKeyState('A') & 0x8000)  MoveEyeTarget(right, -move);
	if (GetAsyncKeyState('E') & 0x8000) { eye.y += move; target.y += move; }
	if (GetAsyncKeyState('Q') & 0x8000) { eye.y -= move; target.y -= move; }

	// マウス回転
	POINT mousePos;
	GetCursorPos(&mousePos);

	if (firstMouse)
	{
		lastMouseX = mousePos.x;
		lastMouseY = mousePos.y;
		firstMouse = false;

		DirectX::XMFLOAT3 dir =
		{
			target.x - eye.x,
			target.y - eye.y,
			target.z - eye.z
		};
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len > 0.0f) { dir.x /= len; dir.y /= len; dir.z /= len; }
		pitch = asinf(std::clamp(dir.y, -1.0f, 1.0f));
		yaw = atan2f(dir.x, dir.z);
	}

	yaw += static_cast<float>(mousePos.x - lastMouseX) * 0.005f;
	pitch -= static_cast<float>(mousePos.y - lastMouseY) * 0.005f;
	pitch = std::clamp(pitch, -DirectX::XM_PIDIV2 + 0.1f, DirectX::XM_PIDIV2 - 0.1f);

	lastMouseX = mousePos.x;
	lastMouseY = mousePos.y;

	// target を eye + 向きベクトルで更新
	target =
	{
		eye.x + cosf(pitch) * sinf(yaw),
		eye.y + sinf(pitch),
		eye.z + cosf(pitch) * cosf(yaw)
	};

	UpdateViewMatrix();
}

// 毎フレームウィンドウサイズを渡して比率を更新する
void StudioCamera::UpdateAspect(float width, float height)
{
	if (height <= 0.0f) return;
	aspect = width / height;
	UpdateProjectionMatrix();
}

void StudioCamera::UpdateViewMatrix()
{
	DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixLookAtLH(Eye, Target, Up));
}
void StudioCamera::UpdateProjectionMatrix()
{
	DirectX::XMStoreFloat4x4(&projection,
		DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(45.0f),
			aspect, 0.1f, 1000.0f));
}

DirectX::XMFLOAT3 StudioCamera::GetFront() const
{
	DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
	DirectX::XMFLOAT3 front;
	DirectX::XMStoreFloat3(&front, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(Target, Eye)));
	return front;
}
DirectX::XMFLOAT3 StudioCamera::GetRight() const
{
	DirectX::XMFLOAT3 front = GetFront();
	DirectX::XMVECTOR Front = DirectX::XMLoadFloat3(&front);
	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMFLOAT3 right;
	DirectX::XMStoreFloat3(&right, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Up, Front)));
	return right;
}