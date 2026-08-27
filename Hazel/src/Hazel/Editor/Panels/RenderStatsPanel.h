#pragma once
#include "Hazel/Scene/Scene.h"
#include "Hazel/Editor/EditorSelection.h"

#include "Hazel/Renderer/RenderDebug/RenderDebug.h"
#include "Hazel/Renderer/RenderTypes.h"

namespace Engine
{
	class RenderStatsPanel
	{
	public:
		static void OnImGuiRender(RenderDebugSetting& debug, RendererLog& log);
	
	private:
		static const char* RenderStatsName[];
	};

	class RenderSettingPanel
	{
	public:
		static void OnImGuiRender(RenderView& view);
	private:
		static const char* RenderQualityPresetNames[];
		static const char* LightingModesName[];
		static const char* ToneMappingOperatorNames[];
	};
}
