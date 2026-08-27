#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hazel/Renderer/RHI/Buffer.h"

namespace Engine
{
	enum class PrimitiveTopology
	{
		Triangles,
		LinesAdjacency
	};

	struct MeshData
	{
		BufferLayout Layout;

		const void* VerticesData = nullptr;
		uint32_t VertexBufferSize = 0;
		uint32_t VertexCount = 0;

		const uint32_t* IndicesData = nullptr;
		uint32_t IndexCount = 0;

		PrimitiveTopology Topology = PrimitiveTopology::Triangles;
	};

	struct MeshVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 tangent;
		glm::vec2 texCoord;
		glm::ivec4 boneIDs;
		glm::vec4 boneWeights;
	};
}
