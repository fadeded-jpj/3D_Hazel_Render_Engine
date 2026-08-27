#include "hzpch.h"
#include "AssetManager.h"

#include "Hazel/Renderer/Model/Import/ModelImporter.h"
#include "Hazel/Animation/VMDHelp/VMDImport.h"
#include "Hazel/Renderer/Resources/RenderResourceCache.h"
#include "Hazel/Renderer/Model/Model.h"
#include "Hazel/AssetsSystem/AssetFileSystem.h"
#include "Hazel/AssetsSystem/AssetMetadataSerializer.h"

#include <cstdint>
#include <random>

namespace Engine
{
	Scope<AssetRegistry> AssetManager::s_AssetRegistry = nullptr;
	AssetManager::AssetsLibrary AssetManager::s_LoadedAssets = AssetManager::AssetsLibrary();
	std::filesystem::path AssetManager::s_AssetRoot = "";

	AssetHandle AssetManager::s_DefaultShaderHandle = 0;
	AssetHandle AssetManager::s_DefaultMaterialHandle = 0;

	void AssetManager::Init(const AssetManagerConfig& config)
	{
		s_AssetRegistry.reset(new AssetRegistry());
		s_LoadedAssets.clear();
		s_AssetRoot = std::filesystem::weakly_canonical(config.GameContentRoot);

		AssetFileSystem::Mount("/Game", s_AssetRoot, false);
		AssetFileSystem::Mount("/Engine", config.EngineContentRoot, true);

		ScanAssets();

		 s_DefaultShaderHandle = ImportAsset("/Engine/Shaders/DefaultShader.glsl");
	}

	void AssetManager::Shutdown()
	{
		s_LoadedAssets.clear();
		s_AssetRegistry.reset();
		AssetFileSystem::Clear();

		s_DefaultShaderHandle = 0;
		s_DefaultMaterialHandle = 0;
		s_AssetRoot.clear();
	}

	AssetHandle AssetManager::ImportAsset(std::string_view path)
	{
		auto assetPath = AssetPath::Parse(path);
		if (!assetPath)
		{
			HZ_CORE_ERROR("Invalid asset path: {0}", std::string(path));
			return 0;
		}
		return ImportAsset(*assetPath);
	}

	AssetHandle AssetManager::ImportAsset(const AssetPath& path)
	{
		auto existing = s_AssetRegistry->Find(path);
		if (existing)
			return existing->Handle;

		const auto absolutePath = AssetFileSystem::Resolve(path);

		auto metaPath = absolutePath;
		metaPath += ".meta";

		AssetMetadata metadata;

		if (std::filesystem::exists(metaPath))
		{
			auto loaded = AssetMetadataSerializer::Deserialize(metaPath);
			if (loaded)
			{
				metadata = std::move(*loaded);
			}
			else
			{
				HZ_CORE_WARN("Invalid asset metadata, regenerating: {0}", metaPath.string());

				metadata = CreateBasicMetadata(path, absolutePath);
				if (!metadata.Valid || metadata.Handle == 0)
					return 0;

				if (!AssetMetadataSerializer::Serialize(metaPath, metadata))
					return 0;
			}

		}
		else
		{
			metadata = CreateBasicMetadata(path, absolutePath);
			if (!metadata.Valid || metadata.Handle == 0)
				return 0;

			if (!AssetMetadataSerializer::Serialize(metaPath, metadata))
			{
				HZ_CORE_ERROR("Failed to write asset metadata: {0}", metaPath.string());
				return 0;
			}
		}

		metadata.ObjectPath = path;
		metadata.FilePath = absolutePath;
		metadata.IsEngineAsset = path.String().rfind("/Engine/", 0) == 0;
		metadata.IsTransient = false;
		metadata.Valid = metadata.Handle != 0 && metadata.Type != AssetType::None;

		if (!metadata.Valid)
		{
			HZ_CORE_ERROR("Invalid metadata content: {0}", metaPath.string());
			return 0;
		}

		if (!s_AssetRegistry->Register(metadata))
			return 0;

		return metadata.Handle;
	}

	bool AssetManager::SetModelSubMeshToonMaterialRole(
		AssetHandle modelHandle,
		uint32_t subMeshIndex,
		ToonMaterialRole role)
	{
		if (role >= ToonMaterialRole::Count)
			return false;

		auto meta = s_AssetRegistry->Find(modelHandle);
		if (!meta || meta->Type != AssetType::Model || meta->IsTransient)
			return false;

		if (!std::holds_alternative<ModelImportSettings>(meta->ImportSettings))
			meta->ImportSettings = ModelImportSettings{};

		auto& settings = std::get<ModelImportSettings>(meta->ImportSettings);
		settings.SubMeshToonMaterialRoles[subMeshIndex] = role;
		meta->modified = SettingModify::User;

		auto metaPath = meta->FilePath;
		metaPath += ".meta";
		if (!AssetMetadataSerializer::Serialize(metaPath, *meta))
		{
			HZ_CORE_ERROR("Failed to write model metadata: {0}", metaPath.string());
			return false;
		}

		return true;
	}

	bool AssetManager::SetModelSubMeshSSRStrength(
		AssetHandle modelHandle,
		uint32_t subMeshIndex,
		float strength)
	{
		if (!std::isfinite(strength))
			return false;

		auto meta = s_AssetRegistry->Find(modelHandle);
		if (!meta || meta->Type != AssetType::Model || meta->IsTransient)
			return false;

		if (!std::holds_alternative<ModelImportSettings>(meta->ImportSettings))
			meta->ImportSettings = ModelImportSettings{};

		auto& settings = std::get<ModelImportSettings>(meta->ImportSettings);
		settings.SubMeshSSRStrengths[subMeshIndex] = std::clamp(strength, 0.0f, 1.0f);
		meta->modified = SettingModify::User;

		auto metaPath = meta->FilePath;
		metaPath += ".meta";
		if (!AssetMetadataSerializer::Serialize(metaPath, *meta))
		{
			HZ_CORE_ERROR("Failed to write model metadata: {0}", metaPath.string());
			return false;
		}

		return true;
	}

	void AssetManager::ScanAssets()
	{
		for (const auto& [virtualRoot, mountPoint] : AssetFileSystem::s_MountPoints)
		{
			ScanMount(mountPoint);
		}
	}

	void AssetManager::ScanMount(const AssetMountPoint& mount)
	{
		if (!std::filesystem::exists(mount.PhysicalPath))
			return;
		
		for (const auto& entry : std::filesystem::recursive_directory_iterator(mount.PhysicalPath))
		{
			if (!entry.is_regular_file())
				continue;

			const auto& physicalPath = entry.path();

			if (physicalPath.extension() == ".meta")
				continue;

			auto relativePath = std::filesystem::relative(physicalPath, mount.PhysicalPath);
#ifdef HZ_PLATFORM_WINDOWS
			std::string relativeString = WideToUtf8(relativePath.native());
			std::replace(relativeString.begin(), relativeString.end(), '\\', '/');
#else
			std::string relativeString = relativePath.generic_string();
#endif

			std::string virtualPath = mount.VirtualPath + "/" + relativeString;

			//HZ_CORE_INFO("virtual path: {0}", virtualPath);

			auto assetPath = AssetPath::Parse(virtualPath);
			if (!assetPath)
				continue;
			if (GetAssetTypeFromPath(*assetPath) == AssetType::None)
				continue;
			
			ImportAsset(*assetPath);
		}
	}

	Ref<Asset> AssetManager::GetAssetInternal(AssetHandle handle)
	{
		auto loaded = s_LoadedAssets.find(handle);
		if (loaded != s_LoadedAssets.end())
			return loaded->second;

		auto metadata = s_AssetRegistry->Find(handle);
		if (metadata == nullptr || !metadata->Valid)
			return nullptr;

		auto asset = LoadAsset(metadata);
		if (!asset)
			return nullptr;

		asset->m_Handle = handle;
		s_LoadedAssets[handle] = asset;

		return asset;
	}
	Ref<Asset> AssetManager::LoadAsset(const Ref<AssetMetadata>& meta)
	{
		auto absolutePath = AssetFileSystem::Resolve(meta->ObjectPath);

		switch (meta->Type)
		{
		case AssetType::AnimationClip:
			return VMDImporter::ImportFromFile(absolutePath.string());
		case AssetType::Model:
		{
			if (std::holds_alternative<ModelImportSettings>(meta->ImportSettings))
			{
				return ModelImporter::ImportFromFile(
					absolutePath,
					std::get<ModelImportSettings>(meta->ImportSettings));
			}
			return ModelImporter::ImportFromFile(absolutePath);
		}
		case AssetType::Texture2D:
		{
			TextureLoadOptions options;
			if (std::holds_alternative<TextureImportSettings>(meta->ImportSettings))
			{
				auto& setting = std::get<TextureImportSettings>(meta->ImportSettings);
				options.ColorSpace = setting.ColorSpace == TextureColorSpace::Auto ? TextureColorSpace::Linear : setting.ColorSpace;
				options.GenerateMips = setting.GenerateMips;
				options.FlipVertically = setting.FlipVertically;
			}
			return Texture2D::Create(absolutePath, options);
		}
		default:
			return nullptr;
		}
	}
	AssetType AssetManager::GetAssetTypeFromPath(const AssetPath& path)
	{
		const std::string& assetPath = path.String();

		const size_t dot = assetPath.find_last_of('.');
		if (dot == std::string::npos)
			return AssetType::None;

		std::string ext = assetPath.substr(dot);

		std::transform(ext.begin(), ext.end(), ext.begin(),[](unsigned char c)
			{
				return static_cast<char>(std::tolower(c));
			});

		if (ext == ".pmx" || ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
			return AssetType::Model;

		if (ext == ".vmd")
			return AssetType::AnimationClip;

		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
			return AssetType::Texture2D;

		if (ext == ".glsl")
			return AssetType::Shader;

		return AssetType::None;
	}
	AssetHandle AssetManager::GenerateUniqueAssetHandle()
	{
		auto fn = []()
			{
				static std::random_device device;
				static std::mt19937_64 generator(device());
				static std::uniform_int_distribution<uint64_t> distribution;

				AssetHandle handle = 0;
				while (handle == 0)
				{
					handle = distribution(generator);
				}

				return handle;
			};

		AssetHandle handle = fn();
		while (s_AssetRegistry->Contains(handle))
			handle = fn();
		return handle;
	}
	AssetMetadata AssetManager::CreateBasicMetadata(const AssetPath& path, const std::filesystem::path& absolutePath)
	{
		if (!std::filesystem::is_regular_file(absolutePath))
		{
			HZ_CORE_ERROR("Asset file does not exist");
			return AssetMetadata();
		}


		auto type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
		{
			HZ_CORE_ERROR("Unsupported asset type: {0}", path.String());
			return AssetMetadata();
		}

		AssetMetadata meta;
		meta.Handle = GenerateUniqueAssetHandle();
		meta.Type = type;
		meta.ObjectPath = path;
		meta.FilePath = absolutePath;
		meta.IsEngineAsset = path.String().rfind("/Engine/", 0) == 0;
		meta.Valid = true;

		switch (type)
		{
		case AssetType::Texture2D:
			meta.ImportSettings = TextureImportSettings{}; break;
		case AssetType::Model:
			meta.ImportSettings = ModelImportSettings{}; break;
		default:
			meta.ImportSettings = std::monostate{}; break;
		}

		return meta;
	}

	template<>
	bool AssetManager::UpdateImportSettings<AssetType::Texture2D>(const AssetImportSetting& setting, const AssetPath& path, SettingModify modify)
	{
		if (!std::holds_alternative<TextureImportSettings>(setting))
			return false;

		auto meta = s_AssetRegistry->Find(path);
		if (!meta)
		{
			ImportAsset(path);
			meta = s_AssetRegistry->Find(path);
		}

		if (!meta || meta->Type != AssetType::Texture2D)
			return false;
		
		auto incoming = std::get<TextureImportSettings>(setting);
		if (meta->modified == SettingModify::User && modify != SettingModify::User)
		{
			HZ_CORE_WARN("Meta has been modified by user: {0}", path.String());
			return false;
		}

		if (!std::holds_alternative<TextureImportSettings>(meta->ImportSettings))
			meta->ImportSettings = TextureImportSettings{};

		auto& current = std::get<TextureImportSettings>(meta->ImportSettings);
		if (modify == SettingModify::User)
		{
			current = incoming;
			meta->modified = SettingModify::User;
		}
		else
		{
			if (current.Usage == TextureUsage::Unknown)
				current.Usage = incoming.Usage;
			if (current.ColorSpace == TextureColorSpace::Auto)
				current.ColorSpace = incoming.ColorSpace;

			current.GenerateMips = incoming.GenerateMips;
			current.FlipVertically = incoming.FlipVertically;

			if (meta->modified == SettingModify::Default)
				meta->modified = modify;
		}
		auto metaPath = meta->FilePath;
		metaPath += ".meta";
		return AssetMetadataSerializer::Serialize(metaPath, *meta);
	}
}
