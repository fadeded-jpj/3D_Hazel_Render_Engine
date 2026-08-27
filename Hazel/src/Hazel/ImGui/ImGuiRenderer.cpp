#include "hzpch.h"
#include "ImGuiRenderer.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLImGuiRenderer.h"

namespace Engine
{
	ImTextureID ImGuiRenderer::GetTextureID(const Ref<Texture2D>& texture)
	{
		HZ_CORE_ASSERT(texture, "Texture is null");
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return ImTextureID();
		case RendererAPI::API::OpenGL:	return OpenGLImGuiRenderer::GetTextureID(texture);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return ImTextureID();
	}
}