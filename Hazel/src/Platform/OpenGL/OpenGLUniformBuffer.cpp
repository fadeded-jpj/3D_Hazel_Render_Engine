#include "hzpch.h"
#include "OpenGLUniformBuffer.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

Engine::OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding)
	:m_Size(size), m_Binding(binding)
{
	glCreateBuffers(1, &m_RenderID);

	glNamedBufferData(m_RenderID, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_RenderID);
}

Engine::OpenGLUniformBuffer::~OpenGLUniformBuffer()
{
	glDeleteBuffers(1, &m_RenderID);
}

void Engine::OpenGLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
	HZ_CORE_ASSERT(offset + size <= m_Size, "Data out of size!");

	glNamedBufferSubData(m_RenderID, offset, size, data);
}

void Engine::OpenGLUniformBuffer::Bind(uint32_t binding) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RenderID);
}
