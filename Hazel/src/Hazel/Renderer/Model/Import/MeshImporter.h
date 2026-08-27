#pragma once

#include "Hazel/Animation/Skeleton.h"
#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"

struct aiMesh;

namespace Engine
{
	class MeshImporter
	{
	public:
		static ImportedMeshData ConvertAiMeshToImportedMeshData(aiMesh* mesh, const Ref<Skeleton>& skeleton);
	};
}
