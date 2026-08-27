#pragma once

// For use by Hazel applications

#include <stdio.h>
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Core/Layer.h"

#include "Hazel/Input/KeyCodes.h"
#include "Hazel/Input/MouseButtonCodes.h"
#include "Hazel/Input/Input.h"

#include "Hazel/Core/Timestep.h"

#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/ImGui/ImGuiRenderer.h"

// ---Renderer-------------------------------
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"

#include "Hazel/Renderer/RHI/FrameBuffer.h"
#include "Hazel/Renderer/RHI/GBuffer.h"

#include "Hazel/Renderer/Geometry/Mesh.h"
#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/Model/Model.h"
#include "Hazel/Renderer/Model/Import/ModelImporter.h"
#include "Hazel/Renderer/RHI/Shader.h"
#include "Hazel/Renderer/RHI/Texture.h"
#include "Hazel/Renderer/RHI/Buffer.h"
#include "Hazel/Renderer/RHI/VertexArray.h"
#include "Hazel/Scene/Scene.h"

#include "Hazel/Camera/CameraController.h"

//------------Animation ------------
#include "Hazel/Animation/VMDHelp/VMDImport.h"

//------------------Assete---------------------
#include "Hazel/AssetsSystem/AssetManager.h"
//----------------------------------------------

//------------ Editor ------------
#include "Hazel/Editor/EditorSelection.h"
#include "Hazel/Editor/Panels/EntityInspectorPanel.h"
#include "Hazel/Editor/Panels/SceneHierarchyPanel.h"
#include "Hazel/Editor/Panels/RenderStatsPanel.h"
