#pragma once

#include "glm/glm.hpp"
#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	// 选择的光照模型
	enum class LightingMode
	{
		PBR, Toon
	};

	// 光源的种类
	enum class LightType : uint8_t
	{
		Directional, Point, Spot
	};

	struct RenderLightResource
	{
		glm::vec4 ColorIntensity;	// (rgb, instensity)
		glm::vec4 PositionRange;	// (world postion, range)
		glm::vec4 DirectionType;	// (light direction, lightType)
		glm::vec4 SpotAngles;		// (cos(inner), cos(outer), _, _)
	};

	struct SceneLightingData
	{
		static constexpr unsigned int MAX_LIGHTS = 16;

		std::array<RenderLightResource, MAX_LIGHTS> Lights;
		unsigned int LightCount = 0;

		glm::vec3 AmbientColor = glm::vec3(1.0f);
		float AmbientIntensity = 0.2f;

		Ref<TextureCubeMap> Skybox = nullptr;
	};
}
