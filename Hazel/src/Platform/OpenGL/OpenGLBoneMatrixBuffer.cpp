#include "hzpch.h"
#include "OpenGLBoneMatrixBuffer.h"

#include "glad/glad.h"

namespace Engine
{
	OpenGLBoneMatrixBuffer::OpenGLBoneMatrixBuffer(uint32_t maxBones)
		:m_MaxBones(maxBones)
	{
		glCreateBuffers(1, &m_RenderID);
		glNamedBufferData(
			m_RenderID,
			m_MaxBones * sizeof(glm::mat4),
			nullptr,
			GL_DYNAMIC_DRAW
		);
	}
	OpenGLBoneMatrixBuffer::~OpenGLBoneMatrixBuffer()
	{
		glDeleteBuffers(1, &m_RenderID);
	}
	void OpenGLBoneMatrixBuffer::SetData(const std::vector<glm::mat4>& mat)
	{
		const size_t count = std::min(mat.size(), static_cast<size_t>(m_MaxBones));

		glNamedBufferSubData(
			m_RenderID,
			0,
			static_cast<GLsizeiptr>(count * sizeof(glm::mat4)),
			mat.data()
		);
	}

	void OpenGLBoneMatrixBuffer::Bind(uint32_t slot)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_RenderID);
	}
}

