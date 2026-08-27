#include "hzpch.h"
#include "AssetFileSystem.h"

namespace Engine
{
	std::unordered_map<std::string, AssetMountPoint> AssetFileSystem::s_MountPoints = std::unordered_map<std::string, AssetMountPoint>();

	void AssetFileSystem::Mount(const AssetPath& virtualRoot, const std::filesystem::path& physicalRoot, bool readOnly)
	{
		AssetMountPoint mp;
		const auto& root = virtualRoot.String();
		if (root.find('/', 1) != std::string::npos)
		{
			HZ_CORE_WARN("Asset mount root must contain exactly one path segment: {0}", root);
			return;
		}

		mp.VirtualPath = root;
		mp.PhysicalPath = std::filesystem::weakly_canonical(physicalRoot);
		mp.ReadOnly = readOnly;
		auto it = s_MountPoints.find(root);
		if(it != s_MountPoints.end())
		{
			HZ_CORE_WARN("Mount point already exists for virtual path: {0}", root);
			return;
		}
		s_MountPoints[root] = std::move(mp);
	}
	bool AssetFileSystem::Mount(std::string_view virtualRoot, const std::filesystem::path& physicalRoot, bool readOnly)
	{
		auto path = AssetPath::Parse(virtualRoot);
		if (!path || path->String().find('/', 1) != std::string::npos)
		{
			HZ_CORE_WARN("Invalid asset mount root: {0}", std::string(virtualRoot));
			return false;
		}

		Mount(*path, physicalRoot, readOnly);
		return true;
	}
	std::filesystem::path AssetFileSystem::Resolve(const AssetPath& path)
	{
		const std::string& virtualPath = path.String();
		const auto separator = virtualPath.find('/', 1);
		const std::string root = virtualPath.substr(0, separator);

		auto it = s_MountPoints.find(root);
		if (it == s_MountPoints.end())
		{
			HZ_CORE_WARN("No mount point found for virtual path: {0}", virtualPath);
			return {};
		}

		const std::string relativePath = separator == std::string::npos
			? std::string()
			: virtualPath.substr(separator + 1);
#ifdef HZ_PLATFORM_WINDOWS
		auto relativePhysicalPath = std::filesystem::path(Utf8ToWide(relativePath));
#else
		auto relativePhysicalPath = std::filesystem::path(relativePath);
#endif

		return (it->second.PhysicalPath / relativePhysicalPath).lexically_normal();
	}

	std::optional<AssetPath> AssetFileSystem::TryMakeAssetPath(const std::filesystem::path& physicalPath)
	{
		std::error_code ec;
		auto absolute = std::filesystem::weakly_canonical(physicalPath, ec);
		if (ec)
			absolute = std::filesystem::absolute(physicalPath, ec);

		for (const auto& [root, mount] : s_MountPoints)
		{
			auto mountRoot = std::filesystem::weakly_canonical(mount.PhysicalPath, ec);
			if (ec)
				mountRoot = std::filesystem::absolute(mount.PhysicalPath, ec);

			auto relative = std::filesystem::relative(absolute, mountRoot, ec);
			if (ec || relative.empty())
				continue;

#ifdef HZ_PLATFORM_WINDOWS
			std::string relativeString = WideToUtf8(relative.native());
			std::replace(relativeString.begin(), relativeString.end(), '\\', '/');
#else
			std::string relativeString = relative.generic_string();
#endif

			if (relativeString.rfind("..", 0) == 0)
				continue;

			auto virtualPath = mount.VirtualPath + "/" + relativeString;
			return AssetPath::Parse(virtualPath);
		}

		return std::nullopt;
	}

	void AssetFileSystem::Clear()
	{
		s_MountPoints.clear();
	}
}
