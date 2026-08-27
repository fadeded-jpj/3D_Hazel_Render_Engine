#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/Geometry/MeshTypes.h"


namespace Engine
{
	class Mesh
	{
	public:
		virtual ~Mesh() {}

		virtual UINT GetVertexCount() const = 0;
		virtual UINT GetIndexCount() const = 0;
		virtual bool IsIndexed() const = 0;

		static Ref<Mesh> Create(const MeshData& data);
	};
}
