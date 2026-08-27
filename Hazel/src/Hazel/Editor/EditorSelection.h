#pragma once

#include "Hazel/Renderer/RenderTypes.h"
#include "Hazel/Scene/Entity.h"


namespace Engine
{
	struct EditorSelection
	{
		Engine::EntityID EntityID = Engine::INVALID_ENTITY_ID;
		uint32_t SubMeshIndex = std::numeric_limits<uint32_t>::max();

		bool HasSubMesh() const;

		RenderObjectID GetRenderObjectID() const;

		void SelectEntity(Engine::EntityID entityID);

		void SelectSubMesh(Engine::EntityID entityID, uint32_t subMeshIndex);

		bool HasEntity() const;
	};
}