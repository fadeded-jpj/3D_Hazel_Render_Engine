#pragma once
#include "Hazel/Renderer/RHI/UniformBuffer.h"

namespace Engine
{
	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding);
		~OpenGLUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind(uint32_t binding) const override;
	private:
		unsigned int m_RenderID = 0;
		uint32_t m_Size;
		uint32_t m_Binding;
	};
}