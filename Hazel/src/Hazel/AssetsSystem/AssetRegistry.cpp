#include "hzpch.h"
#include "AssetRegistry.h"

namespace Engine
{
	bool AssetRegistry::Register(const AssetMetadata& metadata)
	{
		if (!metadata.Valid || metadata.ObjectPath.Empty())
		{
			HZ_CORE_WARN("meta data invalid!");
			return false;
		}

		auto& handle = metadata.Handle;
		const auto& path = metadata.ObjectPath.String();

		auto pathIt = m_PathIndex.find(path);
		if (pathIt != m_PathIndex.end())
		{
			//HZ_CORE_WARN("Path existed");
			return false;
		}


		auto handleIt = m_Assets.find(handle);
		if (handleIt != m_Assets.end())
		{
			HZ_CORE_WARN("Handle existed!");
			return false;
		}

		m_PathIndex.emplace(path, handle);
		m_Assets.emplace(handle, std::make_shared<AssetMetadata>(metadata));
		return true;
	}
	const Ref<AssetMetadata> AssetRegistry::Find(AssetHandle handle) const
	{
		auto it = m_Assets.find(handle);
		if (it == m_Assets.end())
		{
			//HZ_CORE_WARN("Asset not existed!");
			return nullptr;
		}
		return it->second;
	}
	const Ref<AssetMetadata> AssetRegistry::Find(const AssetPath& path) const
	{
		auto it = m_PathIndex.find(path.String());
		if (it == m_PathIndex.end())
		{
			//HZ_CORE_WARN("Asset not existed!");
			return nullptr;
		}
		return Find(it->second);
	}
	bool AssetRegistry::Contains(const AssetHandle& handle)
	{
		return m_Assets.find(handle) != m_Assets.end();
	}
}
