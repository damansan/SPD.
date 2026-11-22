#pragma once
#include <d3d11.h>
#include <string>

// アタッチメント間で共有するデータ
struct StudioContext
{
    // Graphics系（frameworkから借りる）
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;

    // パネル表示フラグ
    bool showAnimator      = false;
    bool showTimeline      = false;
    bool showStateGraph = false;
    bool showSequencer = false;
    bool showAnimationSpeed = false;
    bool showAnimationEdit = false;

    std::string currentModelPath;

    // タイムラインでスクラブ操作されたフラグ
    bool requestPoseUpdate = false;

    // Studioのウィンドウがいるビューポート
    unsigned int studioViewportId = 0;
};