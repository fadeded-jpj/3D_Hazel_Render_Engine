#pragma once
#include "Hazel/Scene/Scene.h"
#include "Hazel/Editor/EditorSelection.h"
namespace Engine
{
	class SceneHierarchyPanel
	{
	public:
		static void OnImGuiRender(const Ref<Scene>& scene, EditorSelection& selection);

	private:
		static void DrawModelNode(const Engine::Ref<Model>& model, const Model::ModelNode& node, EntityID entityID, EditorSelection& selection);
	};
}