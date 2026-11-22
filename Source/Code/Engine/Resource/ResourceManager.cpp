#include "ResourceManager.h"
#include "GLTFImporter.h"
#include "GLTFConverter.h" 
#include "ModelSerializer.h" 
#include "GpuResourceUtils.h" //（GPU再生成用）

#include "Engine/Core/Log.h"
#include "Engine/Resource/AnimationEditBaker.h"

// モデル取得
std::shared_ptr<Model> ResourceManager::GetModel(ID3D11Device* device, const std::string& filename)
{
	// キャッシュ検索
	auto it = modelCache.find(filename);
	if (it != modelCache.end())
	{
		// キャッシュが有効なら使い回す
		return it->second;
	}

	std::shared_ptr<Model> model;

	const std::string cerealPath = ModelSerializer::ToCerealPath(filename);

	// .cereal がある(.cerealを読み込む)
	if (ModelSerializer::Exists(filename))
	{
		Log(" .cereal から読み込み: %s", LogPath(cerealPath).c_str());

		model = std::make_shared<Model>();
		ModelSerializer::Load(*model, ModelSerializer::ToCerealPath(filename));
		GLTFConverter::CreateGpuResources(device, *model);
	}
	// .cereal がない(GLTFを読み込む)
	else
	{
		Log(" glTF から変換: %s -> %s", LogPath(filename).c_str(), LogPath(cerealPath).c_str());

		auto gltfData = GLTFImporter::Load(filename);
		model = GLTFConverter::Convert(device, gltfData);
		ModelSerializer::Save(*model, ModelSerializer::ToCerealPath(filename));
	}
	// アニメーションの編集設定を焼き込む
	if (model->HasSkin())
	{
		// 無くて普通のファイルなので、無いときはログを出さない
		const std::string editPath = AnimationEditData::GetFilePath(filename);
		AnimationEditData editData = editCache.GetOrLoad(editPath,
			[](AnimationEditData& data, const nlohmann::json& j) { data.FromJson(j); }, JsonMissingLog::Silent);

		if (editCache.GetStatus(editPath) == JsonFileStatus::Loaded)
		{
			Log(" アニメーション編集を読み込み: %s", LogPath(editPath).c_str());
		}

		// ルートノードが未設定なら、モデルから探す
		if (editData.rootNodeName.empty())
		{
			editData.rootNodeName = AnimationEditBaker::FindDefaultRootNodeName(*model);
		}

		model->ApplyAnimationEdit(editData);
	}

	// キャッシュに登録
	modelCache[filename] = model;

	return model;
}

// キャッシュ削除
void ResourceManager::ReleaseModel(const std::string& filename)
{
	modelCache.erase(filename);
}

// 全キャッシュ削除
void ResourceManager::ReleaseAll()
{
	modelCache.clear();
}
