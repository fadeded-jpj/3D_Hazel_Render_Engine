#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	//std::filesystem::path Path;
	//TextureColorSpace ColorSpace;
	//bool GenerateMips = true;
	//bool FlipVertically = false;
	struct TextureCacheKey
	{
		std::filesystem::path Path;
		TextureColorSpace ColorSpace;
		bool GenerateMips = true;
		bool FlipVertically = false;

		bool operator==(const TextureCacheKey& other) const
		{
			return Path == other.Path
				&& ColorSpace == other.ColorSpace
				&& GenerateMips == other.GenerateMips
				&& FlipVertically == other.FlipVertically;
		}
	};

	struct TextureCacheKeyHasher
	{
		size_t operator()(const TextureCacheKey& key) const noexcept
		{
			size_t seed =
				std::hash<std::filesystem::path>{}(key.Path);

			auto combine = [&seed](size_t value)
				{
					seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				};

			combine(static_cast<size_t>(key.ColorSpace));
			combine(std::hash<bool>{}(key.GenerateMips));
			combine(std::hash<bool>{}(key.FlipVertically));

			return seed;
		}
	};

	using TextureCache = std::unordered_map<
		TextureCacheKey,
		WeakRef<Texture2D>,
		TextureCacheKeyHasher>;


	//Ref<Shader> Shader;
	//MaterialRenderConfig Config;
	struct MaterialCacheKey
	{
		Ref<Shader> Shader;
		MaterialRenderConfig Config;

		bool operator==(const MaterialCacheKey& other) const
		{
			return Shader == other.Shader && Config.Blend == other.Config.Blend &&
				Config.CastShadow == other.Config.CastShadow && Config.Cull == other.Config.Cull &&
				Config.DepthTest == other.Config.DepthTest && Config.DepthWrite == other.Config.DepthWrite;
		}
	};

	struct MaterialCacheKeyHasher
	{
		size_t operator()(const MaterialCacheKey& key) const noexcept
		{
			size_t seed = std::hash<Shader*>{}(key.Shader.get());

			auto combine = [&seed](size_t value)
				{
					seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				};

			combine(static_cast<size_t>(key.Config.Blend));
			combine(std::hash<bool>{}(key.Config.CastShadow));
			combine(static_cast<size_t>(key.Config.Cull));
			combine(std::hash<bool>{}(key.Config.DepthTest));
			combine(std::hash<bool>{}(key.Config.DepthWrite));

			return seed;
		}
	};
	using MaterialCache = std::unordered_map<MaterialCacheKey, WeakRef<Material>, MaterialCacheKeyHasher>;

	using ShaderCache = std::unordered_map<std::filesystem::path, WeakRef<Shader>>;
}