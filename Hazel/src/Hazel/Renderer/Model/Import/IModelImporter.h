#pragma once

#include <string>

#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"

namespace Engine
{
	class IModelImporter
	{
	public:
		virtual ~IModelImporter() = default;
		virtual ModelImportData ImportFromFile(const std::filesystem::path& filepath) = 0;
	};
}
