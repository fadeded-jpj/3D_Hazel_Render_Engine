#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Engine
{
	struct PMXToonReference
	{
		bool Shared = false;
		int TextureIndex = -1;
		int SharedIndex = -1;
	};

	struct PMXMaterialSupplement
	{
		std::string name;

		unsigned int DrawingFlags = 0;

		glm::vec4 EdgeColor{ 0.0f };
		float EdgeSize = 0.0f;

		int32_t BaseTextureIndex = -1;
		int32_t SphereTextureIndex = -1;
		uint8_t SphereMode = 0;

		PMXToonReference Toon;
	};

	struct PMXMaterialImportData
	{
		std::vector<std::filesystem::path> Textures;
		std::vector<PMXMaterialSupplement> Materials;
	};

	class PMXMaterialReader
	{
	public:
		static PMXMaterialImportData Read(
			const std::filesystem::path& filepath);
	};
}
