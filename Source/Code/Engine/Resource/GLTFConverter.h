#pragma once
#include <memory>
#include <d3d11.h>

#include "GLTFData.h"
#include "Model.h"

class GLTFConverter
{
public:
    // GLTFData を受け取り Model を生成して返す
    static std::shared_ptr<Model> Convert(
        ID3D11Device* device,
        const std::shared_ptr<GLTFData>& data
    );

    // .cereal から復元した後にGPUリソースだけ再生成する
    static void CreateGpuResources(ID3D11Device* device, Model& model);

private:
    // ノード変換
    static void ConvertNodes(
        Model& model,
        const GLTFData& data
    );

    // メッシュ変換（頂点・インデックスバッファ生成）
    static void ConvertMeshes(
        Model& model,
        const GLTFData& data,
        ID3D11Device* device
    );

    // マテリアル変換（テクスチャ生成）
    static void ConvertMaterials(
        Model& model,
        const GLTFData& data,
        ID3D11Device* device
    );

    // アニメーション変換
    static void ConvertAnimations(
        Model& model,
        const GLTFData& data
    );
};