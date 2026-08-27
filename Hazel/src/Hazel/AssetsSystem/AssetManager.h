#pragma once

#include "AssetsManagerType.h"
#include "AssetRegistry.h"
#include "AssetType.h"

#include <string_view>

namespace Engine
{
	struct AssetMountPoint;

	struct AssetManagerConfig
	{
		std::filesystem::path EngineContentRoot;
		std::filesystem::path GameContentRoot;
	};

	class AssetManager
	{
	public:
		static void Init (const AssetManagerConfig& config);
		static void Shutdown();

		static AssetHandle ImportAsset(std::string_view path);
		static AssetHandle ImportAsset(const AssetPath& path);
		static bool SetModelSubMeshToonMaterialRole(
			AssetHandle modelHandle,
			uint32_t subMeshIndex,
			ToonMaterialRole role);
		static bool SetModelSubMeshSSRStrength(
			AssetHandle modelHandle,
			uint32_t subMeshIndex,
			float strength);

		template<typename T>
		static Ref<T> GetAsset(AssetHandle handle);

		template<typename T>
		static Ref<T> GetAsset(std::string_view path);

		template<typename T>
		static Ref<T> GetAsset(const AssetPath& path);
		
		using AssetsLibrary = std::unordered_map<AssetHandle, Ref<Asset>>;

	// 为 meta 导入不同类型的importer 数据
		template<AssetType Type>
		static bool UpdateImportSettings(const AssetImportSetting& setting, const AssetPath& path, SettingModify modify);
	private:
		static Ref<Asset> GetAssetInternal(AssetHandle handle);
		static Ref<Asset> LoadAsset(const Ref<AssetMetadata>& meta);
		static AssetType GetAssetTypeFromPath(const AssetPath& path);

		static AssetHandle GenerateUniqueAssetHandle();

		static AssetMetadata CreateBasicMetadata(const AssetPath& path, const std::filesystem::path& phyPath);

		static void ScanAssets();	// 遍历MountPoint, 只填写.meta 的基础信息
		static void ScanMount(const AssetMountPoint& mountPoint);	// 物理路径转回虚拟路径

	private:
		static Scope<AssetRegistry> s_AssetRegistry;
		static AssetsLibrary s_LoadedAssets;
		static std::filesystem::path s_AssetRoot;

		// Default Assets
		static AssetHandle s_DefaultShaderHandle;
		static AssetHandle s_DefaultMaterialHandle;
	};

	template<typename T>
	inline Ref<T> AssetManager::GetAsset(AssetHandle handle)
	{
		auto asset = GetAssetInternal(handle);
		if (!asset)
			return nullptr;

		if (asset->GetAssetType() != T::GetStaticType())
			return nullptr;
		return std::static_pointer_cast<T>(asset);
	}
	template<typename T>
	inline Ref<T> AssetManager::GetAsset(std::string_view path)
	{
		auto assetPath = AssetPath::Parse(path);
		if (!assetPath)
			return nullptr;
		return GetAsset<T>(*assetPath);
	}

	template<typename T>
	inline Ref<T> AssetManager::GetAsset(const AssetPath& path)
	{
		auto meta = s_AssetRegistry->Find(path);
		if (!meta)
			return nullptr;
		return GetAsset<T>(meta->Handle);
	}
}
