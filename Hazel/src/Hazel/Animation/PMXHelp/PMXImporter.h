#pragma once

#include "Hazel/Animation/Skeleton.h"

namespace Engine
{
	class PMXSkeletonImporter
	{
	public:
		static Ref<Skeleton> ImportFromFile(const std::filesystem::path& filePath);

	private:
		
	};
}
