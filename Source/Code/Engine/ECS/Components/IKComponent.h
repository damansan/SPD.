#pragma once
#include <DirectXMath.h>
#include <vector>

// 足1本分のFootIK設定情報
struct FootIKSettings
{
    // 足のボーンインデックス
    int rootBoneIndex = 74; // thigh_l
    int midBoneIndex = 75;  // calf_l
    int endBoneIndex = 76;  // foot_l

    // レイキャスト関連
    float rayStartOffset = 0.2f;  // レイの始点
    float rayMaxDistance = 0.5f;  // レイの長さ
    float ankleHeight = 0.1f;     // めり込み防止値

    // 曲げる方向
    bool poleVectorForward = true;

	// IKのターゲット位置
    DirectX::XMFLOAT3 targetPos{ 0.0f, 0.0f, 0.0f };

    bool hasInitializedTarget = false;
};

struct IKComponent
{
    // IKを有効にするか
    bool enabled = true; 

    // 足ごとの設定(初期2本)
    std::vector<FootIKSettings> feet{ FootIKSettings{}, FootIKSettings{} };
};