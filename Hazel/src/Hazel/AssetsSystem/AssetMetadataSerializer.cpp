#include "hzpch.h"
#include "AssetMetadataSerializer.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <optional>
#include <charconv>
#include <cstdint>
#include <cmath>

using JSON = nlohmann::json;

namespace Engine
{
	bool AssetMetadataSerializer::Serialize(const std::filesystem::path& metapath, const AssetMetadata& metadata)
	{
		JSON root;

		root["schemaVersion"] = 1;
		root["handle"] = std::to_string(metadata.Handle);
		root["type"] = std::string(AssetTypeToString(metadata.Type));
		root["objectPath"] = metadata.ObjectPath.String();
		root["modified"] = ToString(metadata.modified);

		if (metadata.Type == AssetType::Texture2D &&
			std::holds_alternative<TextureImportSettings>(metadata.ImportSettings))
		{
			const auto& settings = std::get<TextureImportSettings>(metadata.ImportSettings);

			JSON importSettings;
			importSettings["usage"] = ToString(settings.Usage);
			importSettings["colorSpace"] = ToString(settings.ColorSpace);
			importSettings["generateMips"] = settings.GenerateMips;
			importSettings["flipVertically"] = settings.FlipVertically;

			root["importSettings"] = importSettings;
		}
		else if (metadata.Type == AssetType::Model &&
			std::holds_alternative<ModelImportSettings>(metadata.ImportSettings))
		{
			const auto& settings = std::get<ModelImportSettings>(metadata.ImportSettings);

			JSON importSettings;
			importSettings["importMaterials"] = settings.ImportMaterials;
			importSettings["importTextures"] = settings.ImportTextures;
			importSettings["scale"] = settings.Scale;
			importSettings["materialImportMode"] = static_cast<uint32_t>(settings.MaterialMode);

			JSON subMeshToonMaterialRoles = JSON::object();
			for (const auto& [subMeshIndex, role] : settings.SubMeshToonMaterialRoles)
			{
				subMeshToonMaterialRoles[std::to_string(subMeshIndex)] = static_cast<uint32_t>(role);
			}
			importSettings["submeshToonMaterialRoles"] = std::move(subMeshToonMaterialRoles);

			JSON subMeshSSRStrengths = JSON::object();
			for (const auto& [subMeshIndex, strength] : settings.SubMeshSSRStrengths)
			{
				subMeshSSRStrengths[std::to_string(subMeshIndex)] = strength;
			}
			importSettings["submeshSSRStrengths"] = std::move(subMeshSSRStrengths);

			root["importSettings"] = importSettings;
		}

		std::ofstream output(metapath);

		if (!output)
			return false;
		
		output << root.dump(4);

		return output.good();
	}
	std::optional<AssetMetadata> AssetMetadataSerializer::Deserialize(const std::filesystem::path& metaPath)
	{
		std::ifstream input(metaPath);
		if (!input)
			return std::nullopt;

		auto String2Handle = [&](std::string_view str)->std::optional<AssetHandle>
			{
				AssetHandle value = 0;
				auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
				if (ec != std::errc{} || ptr != str.data() + str.size())
					return std::nullopt;
				return value;
			};

		try
		{
			JSON root;
			input >> root;

			AssetMetadata meta;
			auto handle = String2Handle(root.at("handle").get<std::string>());
			if (!handle)
				return std::nullopt;
			meta.Handle = *handle;

			meta.Type = AssetTypeFromString(root.at("type").get<std::string>());

			auto objpath = AssetPath::Parse(root.at("objectPath").get<std::string>());
			if (!objpath)
				return std::nullopt;
			meta.ObjectPath = *objpath;

			meta.Valid = true;

			if (root.contains("modified"))
			{
				auto modified = FromString<SettingModify>(root["modified"].get<std::string>());
				meta.modified = modified.value_or(SettingModify::Default);
			}

			if (root.contains("importSettings"))
			{
				const auto& importSettings = root["importSettings"];

				if (meta.Type == AssetType::Texture2D)
				{
					TextureImportSettings settings;

					if (importSettings.contains("usage"))
					{
						auto usage = FromString<TextureUsage>(importSettings["usage"].get<std::string>());
						settings.Usage = usage.value_or(TextureUsage::Unknown);
					}

					if (importSettings.contains("colorSpace"))
					{
						auto colorSpace = FromString<TextureColorSpace>(importSettings["colorSpace"].get<std::string>());
						settings.ColorSpace = colorSpace.value_or(TextureColorSpace::Auto);
					}

					settings.GenerateMips = importSettings.value("generateMips", true);
					settings.FlipVertically = importSettings.value("flipVertically", false);

					meta.ImportSettings = settings;
				}
				else if (meta.Type == AssetType::Model)
				{
					ModelImportSettings settings;
					settings.ImportMaterials = importSettings.value("importMaterials", true);
					settings.ImportTextures = importSettings.value("importTextures", true);
					settings.Scale = importSettings.value("scale", 1.0f);

					const auto materialMode = importSettings.value(
						"materialImportMode", static_cast<uint32_t>(MaterialImportMode::Auto));
					if (materialMode <= static_cast<uint32_t>(MaterialImportMode::Toon))
						settings.MaterialMode = static_cast<MaterialImportMode>(materialMode);

					if (importSettings.contains("submeshToonMaterialRoles"))
					{
						const auto& roles = importSettings["submeshToonMaterialRoles"];
						if (!roles.is_object())
							return std::nullopt;

						for (const auto& [indexString, roleJson] : roles.items())
						{
							uint32_t subMeshIndex = 0;
							auto [ptr, ec] = std::from_chars(
								indexString.data(), indexString.data() + indexString.size(), subMeshIndex);
							if (ec != std::errc{} || ptr != indexString.data() + indexString.size())
								continue;

							const int roleValue = roleJson.get<int>();
							if (roleValue < 0 || roleValue >= static_cast<int>(ToonMaterialRole::Count))
								continue;

							settings.SubMeshToonMaterialRoles[subMeshIndex] =
								static_cast<ToonMaterialRole>(roleValue);
						}
					}

					if (importSettings.contains("submeshSSRStrengths"))
					{
						const auto& strengths = importSettings["submeshSSRStrengths"];
						if (!strengths.is_object())
							return std::nullopt;

						for (const auto& [indexString, strengthJson] : strengths.items())
						{
							uint32_t subMeshIndex = 0;
							auto [ptr, ec] = std::from_chars(
								indexString.data(), indexString.data() + indexString.size(), subMeshIndex);
							if (ec != std::errc{} || ptr != indexString.data() + indexString.size() ||
								!strengthJson.is_number())
							{
								continue;
							}

							const float strength = strengthJson.get<float>();
							if (!std::isfinite(strength))
								continue;

							settings.SubMeshSSRStrengths[subMeshIndex] =
								std::clamp(strength, 0.0f, 1.0f);
						}
					}

					meta.ImportSettings = settings;
				}
			}
			else
			{
				// 旧 meta 或基础 meta，给默认 settings
				switch (meta.Type)
				{
				case AssetType::Texture2D:
					meta.ImportSettings = TextureImportSettings{};
					break;
				case AssetType::Model:
					meta.ImportSettings = ModelImportSettings{};
					break;
				default:
					meta.ImportSettings = std::monostate{};
					break;
				}
			}

			return meta;
		}
		catch (const JSON::exception& e)
		{
			HZ_CORE_ERROR("Failed to read asset metadata {0}: {1}", metaPath.string(), e.what());
			return std::nullopt;
		}
	}
}
