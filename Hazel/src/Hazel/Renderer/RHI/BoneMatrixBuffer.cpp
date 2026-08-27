#include "hzpch.h"
#include "BoneMatrixBuffer.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLBoneMatrixBuffer.h"

namespace Engine
{
	Ref<BoneMatrixBuffer> BoneMatrixBuffer::Create(uint32_t maxBones)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		HZ_CORE_ASSERT(false, "RendererAPI::NONE is not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLBoneMatrixBuffer>(maxBones);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
