#pragma once

#include "AssetsManagerType.h"

namespace Engine
{
	class AssetRegistry
	{
	public:
		bool Register(const AssetMetadata& metadata);

		const Ref<AssetMetadata> Find(AssetHandle handle) const;
		const Ref<AssetMetadata> Find(const AssetPath& path) const;
		bool Contains(const AssetHandle& handle);

	private:
		std::unordered_map<AssetHandle, Ref<AssetMetadata>> m_Assets;
		std::unordered_map<std::string, AssetHandle> m_PathIndex;
	};
}
