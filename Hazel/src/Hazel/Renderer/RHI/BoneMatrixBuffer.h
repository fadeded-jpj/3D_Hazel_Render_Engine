#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Hazel/Core/Core.h"

namespace Engine
{
	class BoneMatrixBuffer
	{
	public:
		virtual ~BoneMatrixBuffer() = default;

		virtual void SetData(const std::vector<glm::mat4>& mat) = 0;
		virtual void Bind(uint32_t slot) = 0;

		static Ref<BoneMatrixBuffer> Create(uint32_t maxBones);
	};
}
