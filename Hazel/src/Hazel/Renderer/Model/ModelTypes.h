#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hazel/Core/Core.h"

namespace Engine
{
	class Mesh;

	struct AABB
	{
		glm::vec3 Min{ 0.0f };
		glm::vec3 Max{ 0.0f };
	};

	struct BoundingSphere
	{
		glm::vec3 Origin{ 0.0f };
		float Radius = 0.0f;
	};

	struct Bounds
	{
		glm::vec3 Origin{ 0.0f };
		glm::vec3 BoxExtent{ 0.0f };
		float SphereRadius = 0.0f;
	};

	struct SubMesh
	{
		Ref<Engine::Mesh> Mesh;
		Ref<Engine::Mesh> OutlineEdgeMesh;
		AABB LocalBounds;

		uint32_t MaterialIndex = 0;
		std::string Name;
	};

	struct ModelNode
	{
		std::string Name;
		glm::mat4 LocalTransform = glm::mat4(1.0f);
		std::vector<uint32_t> MeshIndices;
		std::vector<ModelNode> Children;
	};
}
