#include "hzpch.h"
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

#include "OpenGLMesh.h"
#include "Hazel/Renderer/RHI/VertexArray.h"

namespace Engine
{
	void OpenGLRendererAPI::Init()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		glDisable(GL_BLEND);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}
	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}
	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
	}
	void OpenGLRendererAPI::DrawMesh(const Ref<Mesh>& Mesh)
	{
		auto glMesh = std::dynamic_pointer_cast<OpenGLMesh>(Mesh);
		HZ_CORE_ASSERT(glMesh, "Mesh is not an OpenGLMesh!");

		glMesh->Bind();
		if (glMesh->IsIndexed())
			glDrawElements(glMesh->GetMode(), glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(glMesh->GetMode(), 0, glMesh->GetVertexCount());
	}

	void OpenGLRendererAPI::DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
	{
		glDispatchCompute(groupX, groupY, groupZ);
	}

	void OpenGLRendererAPI::InsertMemoryBarrier(MemoryBarrierBits barriers)
	{
		if (barriers == MemoryBarrierBits::None)
			return;

		if (barriers == MemoryBarrierBits::All)
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			return;
		}

		GLbitfield glBarriers = 0;
		const uint32_t bits = static_cast<uint32_t>(barriers);
		if (bits & static_cast<uint32_t>(MemoryBarrierBits::ShaderImageAccess))
			glBarriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
		if (bits & static_cast<uint32_t>(MemoryBarrierBits::TextureFetch))
			glBarriers |= GL_TEXTURE_FETCH_BARRIER_BIT;
		if (bits & static_cast<uint32_t>(MemoryBarrierBits::ShaderStorage))
			glBarriers |= GL_SHADER_STORAGE_BARRIER_BIT;

		if (glBarriers != 0)
			glMemoryBarrier(glBarriers);
	}
	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}
	void OpenGLRendererAPI::SetBlend(BlendMode mode)
	{
		switch (mode)
		{
		case BlendMode::Opaque:
		case BlendMode::AlphaCutout:
			glDisable(GL_BLEND);
			break;

		case BlendMode::AlphaBlend:
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// 对 RGB 统一混合因子
			break;

		case BlendMode::Add:
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			// (src RGB factor, target RGB factor, src Alpha factor, target Alpha factor)
			// 所以最终结果是： (Src.rgb + target.rgb, target.alpha)
			glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);	
			break;

		default:
			HZ_CORE_ASSERT(false, "Unknown BlendMode");
			break;
		}
	}
	void OpenGLRendererAPI::SetCull(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::None:
			glDisable(GL_CULL_FACE);
			break;

		case CullMode::Back:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;

		case CullMode::Front:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;

		default:
			HZ_CORE_ASSERT(false, "Unknown CullMode");
			break;
		}
	}
	void OpenGLRendererAPI::SetDepthTest(bool flag)
	{
		if (flag)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}
	void OpenGLRendererAPI::SetDepthWrite(bool flag)
	{
		glDepthMask(flag ? GL_TRUE : GL_FALSE);
	}
	void OpenGLRendererAPI::SetPolygonMode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode::Fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case PolygonMode::Line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		default:
			HZ_CORE_ASSERT(false, "Unknown PolygonMode");
			break;
		}
	}
	void OpenGLRendererAPI::SetPolygonOffset(bool enabled, float x, float y)
	{
		if (enabled)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(x, y);
			return;
		}
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	void OpenGLRendererAPI::PushDebugGroup(std::string_view name)
	{
#ifdef HZ_DEBUG
		glPushDebugGroup(
			GL_DEBUG_SOURCE_APPLICATION, 0,
			static_cast<GLsizei>(name.size()),
			name.data()
			);
#endif // HZ_DEBUG

	}
	void OpenGLRendererAPI::PopDebugGroup()
	{
#ifdef HZ_DEBUG
		glPopDebugGroup();
#endif // HZ_DEBUG

	}
}
