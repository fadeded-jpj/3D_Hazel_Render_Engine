#pragma once
#include "UniformBuffer.h"

#include "glm/glm.hpp"

namespace Engine
{
	enum class UniformBufferBinding : unsigned int
	{
		Camera = 0,
		Lighting = 1,
		Shadow = 2
	};

	struct alignas(16) CameraUniformData
	{
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 InverseProjection;

		glm::mat4 ViewProj;
		glm::mat4 InverseViewProj;

		glm::mat4 PreViewProj;

		glm::mat4 JitteredViewProj;
		glm::mat4 InverseJitteredViewProj;

		glm::mat4 StabledViewProj;
		glm::mat4 InverseStabledViewProj;

		glm::vec4 CameraPositionAndTime;
		glm::vec4 ViewportSizeAndJittered;
		glm::vec4 CameraClip;
	};
	static_assert(sizeof(CameraUniformData) == 688,
		"CameraUniformData must match CameraBlock's std140 layout");
}
