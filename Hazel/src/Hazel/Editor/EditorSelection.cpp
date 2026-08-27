#include "hzpch.h"
#include "EditorSelection.h"

#include "Hazel/Renderer/Renderer.h"

namespace Engine
{
	bool EditorSelection::HasSubMesh() const
	{
		return EntityID != Engine::INVALID_ENTITY_ID &&
			SubMeshIndex != std::numeric_limits<uint32_t>::max();
	}

	Engine::RenderObjectID EditorSelection::GetRenderObjectID() const
	{
		return HasSubMesh()
			? Engine::Renderer::MakeRenderObjectID(EntityID, SubMeshIndex)
			: Engine::INVALID_RENDER_OBJECT_ID;
	}

	void EditorSelection::SelectEntity(Engine::EntityID entityID)
	{
		EntityID = entityID;
		SubMeshIndex = std::numeric_limits<uint32_t>::max();
	}

	void EditorSelection::SelectSubMesh(Engine::EntityID entityID, uint32_t subMeshIndex)
	{
		EntityID = entityID;
		SubMeshIndex = subMeshIndex;
	}

	bool EditorSelection::HasEntity() const
	{
		return EntityID != Engine::INVALID_ENTITY_ID;
	}
}