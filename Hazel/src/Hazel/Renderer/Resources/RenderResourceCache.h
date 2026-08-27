#pragma once

#include "ResourceType.h"

namespace Engine
{
	class Shader;
	class Texture;

	class RenderResourceCache
	{
	public:
		static void Init();
		static void Shutdown();
		static void Clear();

		static Ref<Shader> GetShader(const std::filesystem::path& filepath);
		static Ref<Texture> GetTexture(const std::filesystem::path& filepath, const TextureLoadOptions& options = {});
		static Ref<Material> GetMaterial(const Ref<Shader>& shader, const MaterialRenderConfig& config);
	private:
		static std::filesystem::path GetNormalizeAsseetPath(const std::filesystem::path& filepath);
	private:
		static TextureCache s_TextureCache;
		static MaterialCache s_MaterialCache;
		static ShaderCache s_ShaderCache;

	};
}