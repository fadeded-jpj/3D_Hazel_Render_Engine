#pragma once

#include "AssetPath.h"

namespace Engine
{
	struct AssetMountPoint
	{
		std::string VirtualPath;
		std::filesystem::path PhysicalPath;
		bool ReadOnly = false;
	};


	// Engine context : /Engine/Path/To/Asset
	// Project context: /Game/Path/To/Asset
	class AssetFileSystem
	{
	public:
		static void Mount(const AssetPath& virtualRoot, const std::filesystem::path& physicalRoot, bool readOnly = false);
		static bool Mount(std::string_view virtualRoot, const std::filesystem::path& physicalRoot, bool readOnly = false);
		static std::filesystem::path Resolve(const AssetPath& virtualPath);
		static std::optional<AssetPath> TryMakeAssetPath(const std::filesystem::path& physicalPath);
		static void Clear();
	private:
		static std::unordered_map<std::string, AssetMountPoint> s_MountPoints;
		friend class AssetManager;
	};
}
