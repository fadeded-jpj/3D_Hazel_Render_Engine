#pragma once

#include "Hazel/Scene/Scene.h"
#include "Hazel/Editor/EditorSelection.h"

namespace Engine
{
	class EntityInspectorPanel
	{
	public:
		static void OnImGuiRender(const Ref<Scene>& scene, EditorSelection& selection);

	private:
		static void TransformRender(TransformComponent* transform);
		static void LightRender(LightComponent* light);
		static void SubMeshRender(RenderComponent* render, uint32_t subMeshIndex);

	private:
		static const char* blendNames[]; 
		static const char* cullNames[]; 
		static const char* lightNames[];
	};
}
