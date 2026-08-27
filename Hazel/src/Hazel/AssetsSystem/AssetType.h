#pragma once

#include "AssetsManagerType.h"

namespace Engine
{
	class Asset
	{
	public:
		virtual ~Asset() = default;

		inline AssetHandle GetHandle() const { return m_Handle; }
		virtual AssetType GetAssetType() const = 0;

	private:
		AssetHandle m_Handle = 0;
		friend class AssetManager;
	};

#define ASSET_TYPE(type)				\
	static AssetType GetStaticType()	\
	{									\
		return AssetType::type;			\
	}									\
	AssetType GetAssetType() const override {return GetStaticType();}
}