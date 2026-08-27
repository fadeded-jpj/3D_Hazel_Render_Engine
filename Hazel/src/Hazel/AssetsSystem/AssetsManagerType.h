#pragma once

#include <filesystem>
#include <unordered_map>

#include "AssetPath.h"
#include "Hazel/Core/Helper.h"
#include "Hazel/Renderer/Material/MaterialType.h"
#include "Hazel/Renderer/RHI/TextureType.h"

namespace Engine
{
	using AssetHandle = uint64_t;

#define ASSET_TYPE_LIST(x)	\
	X(None)					\
	X(Model)				\
	X(AnimationClip)		\
	X(Texture2D)			\
	X(TextureCubeMap)		\
	X(Texture2DArray)		\
	X(Shader)				\
	X(Material)	

	enum class AssetType
	{
#define X(name) name,
		ASSET_TYPE_LIST(x)
#undef X
	};

	inline const char* AssetTypeToString(AssetType type)
	{
		switch (type)
		{
#define X(name) case AssetType::name: return #name;
			ASSET_TYPE_LIST(x)
#undef X
		}
		return "None";
	}

	inline AssetType AssetTypeFromString(const std::string& str)
	{
#define X(name) if(str == #name) return AssetType::name;
		ASSET_TYPE_LIST(x)
#undef X
			return AssetType::None;
	}


	//----------------------  public Import Setting ---------------------
#define LIST(X) \
    X(Default)                \
    X(Auto)           \
    X(User)

	enum class SettingModify
	{
#define X(name) name,
		LIST(X)
#undef X
	};

	inline std::string ToString(SettingModify queue)
	{
		switch (queue)
		{
#define X(name) case SettingModify::name: return #name;
			LIST(X)
#undef X

		default: return "Unknown";
		}
	}
	template<>
	inline std::optional<SettingModify> FromString<SettingModify>(std::string str)
	{

#define X(name) if(str == #name) return SettingModify::name;
		LIST(X);
#undef X
			return std::nullopt;
	}

#undef LIST



	// ------------------- Model Import Setting -------------------------

	struct ModelImportSettings
	{
		bool ImportMaterials = true;
		bool ImportTextures = true;
		float Scale = 1.0f;
		MaterialImportMode MaterialMode = MaterialImportMode::Auto;
		std::unordered_map<uint32_t, ToonMaterialRole> SubMeshToonMaterialRoles;
		std::unordered_map<uint32_t, float> SubMeshSSRStrengths;
	};

	// -----------------------  Texture Import Setting -----------------
#define LIST(X) \
    X(Unknown)             \
    X(BaseColor)           \
    X(Normal)			\
	X(Data)

	enum class TextureUsage
	{
#define X(name) name,
		LIST(X)
#undef X
	};

	inline std::string ToString(TextureUsage queue)
	{
		switch (queue)
		{
#define X(name) case TextureUsage::name: return #name;
			LIST(X)
#undef X

		default: return "Unknown";
		}
	}
	template<>
	inline std::optional<TextureUsage> FromString<TextureUsage>(std::string str)
	{

#define X(name) if(str == #name) return TextureUsage::name;
		LIST(X);
#undef X
		return std::nullopt;
	}

#undef LIST

	struct TextureImportSettings
	{
		TextureUsage Usage = TextureUsage::Unknown;
		TextureColorSpace ColorSpace = TextureColorSpace::Auto;

		bool GenerateMips = true;
		bool FlipVertically = false;
	};

	using AssetImportSetting = std::variant <
		std::monostate,
		TextureImportSettings,
		ModelImportSettings
	>;

	// ----------------------- Asset Metadata ----------------------------

	//AssetHandle Handle = 0;
	//AssetType Type = AssetType::None;
	//AssetPath ObjectPath;				// virtual Path
	//std::filesystem::path FilePath;		// physical Path
	//bool IsEngineAsset = false;
	//bool IsTransient = false;
	//bool Valid = false;
	struct AssetMetadata
	{
		AssetHandle Handle = 0;
		AssetType Type = AssetType::None;

		AssetPath ObjectPath;				// virtual Path
		std::filesystem::path FilePath;		// physical Path

		bool IsEngineAsset = false;
		bool IsTransient = false;
		bool Valid = false;

		SettingModify modified = SettingModify::Default;

		AssetImportSetting ImportSettings;
	};
}
