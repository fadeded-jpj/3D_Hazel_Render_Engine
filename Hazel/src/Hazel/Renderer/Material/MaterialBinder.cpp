#include "hzpch.h"
#include "MaterialBinder.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RHI/RendererAPI.h"
#include "Platform/OpenGL/OpenGLMaterial.h"

namespace Engine
{
	Scope<MaterialBinder> MaterialBinder::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None: HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL: return std::make_unique<OpenGLMaterialBinder>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
