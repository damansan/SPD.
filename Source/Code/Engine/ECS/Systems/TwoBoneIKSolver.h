#pragma once
#include <DirectXMath.h>

// 2BoneIKソルバー
// root-mid-end の3関節を、target(足首を置きたい位置)に届かせる
namespace TwoBoneIKSolver
{
    struct Result
    {
        DirectX::XMFLOAT3 midPosition;      // 解いた後のmid(膝)のワールド位置
        DirectX::XMFLOAT4 rootRotation;     // 解いた後のroot(腰)のワールド回転
        DirectX::XMFLOAT4 midRotation;      // 解いた後のmid(膝)のワールド回転
        DirectX::XMFLOAT3 endPosition;      // 実際に置かれたend(足首)の位置(クランプ後のtarget)
    };

    // root, mid, end : 現在(IK適用前)の3関節のワールド位置
    // rootRotation, midRotation : 現在のroot, midのワールド回転(クォータニオン)
    // target : end(足首)を届かせたいワールド座標
    // poleVector : mid(膝)を向けたい方向のワールド座標(位置ではなく方向を示す1点)
    Result Solve(
        const DirectX::XMFLOAT3& root,
        const DirectX::XMFLOAT3& mid,
        const DirectX::XMFLOAT3& end,
        const DirectX::XMFLOAT4& rootRotation,
        const DirectX::XMFLOAT4& midRotation,
        const DirectX::XMFLOAT3& target,
        const DirectX::XMFLOAT3& poleVector);

    // fromベクトルをtoベクトルに向けて、最短回転のクォータニオンを求める
    DirectX::XMVECTOR RotationFromToRotation(DirectX::FXMVECTOR from, DirectX::FXMVECTOR to);
}