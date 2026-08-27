#include "hzpch.h"
#include "Mesh.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLMesh.h"

namespace Engine
{
	Ref<Mesh> Mesh::Create(const MeshData& data)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:	HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLMesh>(data);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
