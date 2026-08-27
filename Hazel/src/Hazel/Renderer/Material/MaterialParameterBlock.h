#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Hazel/Core/Core.h"

namespace Engine
{
	class Texture;

	struct MaterialParameterBlock
	{
		std::unordered_map<std::string, int> Ints;
		std::unordered_map<std::string, float> Floats;
		std::unordered_map<std::string, glm::vec2> Float2s;
		std::unordered_map<std::string, glm::vec3> Float3s;
		std::unordered_map<std::string, glm::vec4> Float4s;
		std::unordered_map<std::string, glm::mat3> Mat3s;
		std::unordered_map<std::string, glm::mat4> Mat4s;
	};

	struct MaterialTextureBinding
	{
		Ref<Texture> Resource;
		unsigned int LocalSlot = 0;
	};
}
