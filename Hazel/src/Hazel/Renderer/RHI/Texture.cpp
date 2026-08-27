#include <hzpch.h>
#include "Texture.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Engine
{
	//Ref<Texture2D> Texture2D::Create(const std::string& path)
	//{
	//	switch (Renderer::GetAPI())
	//	{
	//	case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
	//	case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTexture2D>(path);
	//	}
	//	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	//	return nullptr;
	//}
	Ref<Texture2D> Texture2D::Create(const std::filesystem::path& path, const TextureLoadOptions& options)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTexture2D>(path, options);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	Ref<Texture2D> Texture2D::Create(int width, int height, TextureFormat format,
		uint32_t mipLevels, TextureWrap wrap)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTexture2D>(width, height, format, mipLevels, wrap);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<TextureCubeMap> TextureCubeMap::Create(const std::filesystem::path& path, const TextureLoadOptions& options)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTextureCubeMap>(path, options);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<TextureCubeMap> TextureCubeMap::Create(unsigned int size, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTextureCubeMap>(size, format);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	Ref<Texture2DArray> Texture2DArray::Create(unsigned int width, unsigned int height, unsigned int count, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTexture2DArray>(width, height, count, format);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
