#include "hzpch.h"
#include "RenderResourceCache.h"

namespace Engine
{
	TextureCache RenderResourceCache::s_TextureCache = TextureCache();
	MaterialCache RenderResourceCache::s_MaterialCache = MaterialCache();
	ShaderCache RenderResourceCache::s_ShaderCache = ShaderCache();

	void RenderResourceCache::Init()
	{
		s_TextureCache = {};
		s_MaterialCache = {};
		s_ShaderCache = {};
	}
	void RenderResourceCache::Shutdown()
	{
		Clear();
	}
	void RenderResourceCache::Clear()
	{
		s_MaterialCache.clear();
		s_ShaderCache.clear();
		s_TextureCache.clear();
	}

	Ref<Shader> RenderResourceCache::GetShader(const std::filesystem::path& filepath)
	{
		auto normalizedPath = GetNormalizeAsseetPath(filepath);;
		auto it = s_ShaderCache.find(normalizedPath);
		if (it == s_ShaderCache.end())
		{
			auto shader = Shader::Create(normalizedPath.string());
			it = s_ShaderCache.emplace(normalizedPath, std::move(shader)).first;

			return it->second.lock();
		}
		else if (it->second.expired())
		{
			auto shader = Shader::Create(normalizedPath.string());
			it->second = std::move(shader);

			return it->second.lock();
		}

		return it->second.lock();
	}
	Ref<Texture> RenderResourceCache::GetTexture(const std::filesystem::path& filepath, const TextureLoadOptions& options)
	{
		auto normalizedPath = GetNormalizeAsseetPath(filepath);;
		TextureCacheKey key{ normalizedPath, options.ColorSpace, options.GenerateMips, options.FlipVertically };
		auto it = s_TextureCache.find(key);
		if (it == s_TextureCache.end())
		{
			auto texture = Texture2D::Create(normalizedPath, options);
			it = s_TextureCache.emplace(key, std::move(texture)).first;

			return it->second.lock();
		}
		else if (it->second.expired())
		{
			auto texture = Texture2D::Create(normalizedPath, options);
			it->second = std::move(texture);

			return it->second.lock();
		}

		return it->second.lock();
	}
	Ref<Material> RenderResourceCache::GetMaterial(const Ref<Shader>& shader, const MaterialRenderConfig& config)
	{
		MaterialCacheKey key{ shader, config };

		auto it = s_MaterialCache.find(key);
		if (it == s_MaterialCache.end())
		{
			auto material = Material::Create(shader, config);
			it = s_MaterialCache.emplace(key, std::move(material)).first;

			return it->second.lock();
		}
		else if (it->second.expired())
		{
			auto material = Material::Create(shader, config);
			it->second = std::move(material);

			return it->second.lock();
		}

		return it->second.lock();
	}
	std::filesystem::path RenderResourceCache::GetNormalizeAsseetPath(const std::filesystem::path& filepath)
	{
		return filepath.lexically_normal();
	}
}