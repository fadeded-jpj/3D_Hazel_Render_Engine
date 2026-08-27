#pragma once

#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"

namespace Engine
{
	class OpenGLBoneMatrixBuffer : public BoneMatrixBuffer
	{
	public:
		OpenGLBoneMatrixBuffer(uint32_t maxBones);
		~OpenGLBoneMatrixBuffer();

		// 通过 BoneMatrixBuffer 继承
		void SetData(const std::vector<glm::mat4>& mat) override;

		void Bind(uint32_t slot) override;
	private:
		uint32_t m_MaxBones = 0;
		unsigned int m_RenderID = 0;

	};
}
