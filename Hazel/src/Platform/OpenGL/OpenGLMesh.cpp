#include "hzpch.h"
#include "OpenGLMesh.h"

namespace Engine
{
	OpenGLMesh::OpenGLMesh(const MeshData& data)
        : m_VertexCount(data.VertexCount), m_IndexCount(data.IndexCount)
    {
        m_Mode = data.Topology == PrimitiveTopology::LinesAdjacency ? GL_LINES_ADJACENCY : GL_TRIANGLES;

        m_VertexArray = std::move(VertexArray::Create());

        
        auto VB = Engine::VertexBuffer::Create((float*)data.VerticesData, data.VertexBufferSize);

        VB->SetLayout(data.Layout);
        m_VertexArray->AddVertexBuffer(VB);

        if (IsIndexed())
        {
            auto IB = Engine::IndexBuffer::Create((UINT*)data.IndicesData, data.IndexCount);
            m_VertexArray->SetIndexBuffer(IB);
        }
    }
    UINT OpenGLMesh::GetVertexCount() const
    {
		return m_VertexCount;
    }

    UINT OpenGLMesh::GetIndexCount() const
    {
        return m_IndexCount;
    }

    bool OpenGLMesh::IsIndexed() const
    {
        return m_IndexCount > 0;
    }

    void OpenGLMesh::Bind() const
    {
        m_VertexArray->Bind();
	}

    void OpenGLMesh::UnBind() const
    {
        m_VertexArray->Unbind();
    }
}

