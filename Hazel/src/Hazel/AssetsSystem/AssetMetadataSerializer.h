#pragma once

#include "Hazel/AssetsSystem/AssetsManagerType.h"
#include "Hazel/AssetsSystem/AssetPath.h"

namespace Engine
{
	class AssetMetadataSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& metapath, const AssetMetadata& metadata);
		static std::optional<AssetMetadata> Deserialize(const std::filesystem::path& metaPath);
	};
}