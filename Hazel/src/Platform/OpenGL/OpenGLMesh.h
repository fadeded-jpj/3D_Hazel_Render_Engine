#pragma once

#include "Hazel/Renderer/Geometry/Mesh.h"
#include "Hazel/Renderer/RHI/VertexArray.h"

//#include "GLFW/glfw3.h"
#include "glad/glad.h"

namespace Engine
{
	class OpenGLMesh : public Mesh
	{
	public:
		OpenGLMesh(const MeshData& data);

		virtual UINT GetVertexCount() const override;
		virtual UINT GetIndexCount() const override;
		virtual bool IsIndexed() const override;

		void Bind() const;
		void UnBind() const;

		GLenum GetMode() const { return m_Mode; }

	private:
		Ref<VertexArray> m_VertexArray;
		UINT m_VertexCount = 0;
		UINT m_IndexCount = 0;

		GLenum m_Mode = GL_TRIANGLES;
	};
}
