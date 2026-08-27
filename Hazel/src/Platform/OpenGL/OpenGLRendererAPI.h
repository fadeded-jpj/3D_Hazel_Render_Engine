#pragma once

#include "Hazel/Renderer/RHI/RendererAPI.h"
#include <glad/glad.h>

namespace Engine
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;
	
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
		virtual void DrawMesh(const Ref<Mesh>& Mesh) override;
		virtual void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override;
		virtual void InsertMemoryBarrier(MemoryBarrierBits barriers) override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetBlend(BlendMode mode) override;
		virtual void SetCull(CullMode mode) override;
		virtual void SetDepthTest(bool flag) override;
		virtual void SetDepthWrite(bool flag) override;
		virtual void SetPolygonMode(PolygonMode mode) override;
		virtual void SetPolygonOffset(bool enabled, float x, float y) override;

		virtual void PushDebugGroup(std::string_view name) override;
		virtual void PopDebugGroup() override;

		virtual uint32_t CreateTimestampQuery() override
		{
			GLuint query = 0;
			glGenQueries(1, &query);
			return query;
		}

		virtual void DeleteTimestampQuery(uint32_t query) override
		{
			glDeleteQueries(1, &query);
		}

		virtual void WriteTimestamp(uint32_t query) override
		{
			glQueryCounter(query, GL_TIMESTAMP);
		}

		virtual bool IsTimestampAvailable(uint32_t query) override
		{
			GLint available = GL_FALSE;
			glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
			return available == GL_TRUE;
		}

		virtual uint64_t GetTimestamp(uint32_t query) override
		{
			GLuint64 timestamp = 0;
			glGetQueryObjectui64v(query, GL_QUERY_RESULT, &timestamp);
			return timestamp;
		}
	};

}


