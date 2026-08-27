#pragma once

#include <string_view>

#include "Hazel/Renderer/RHI/RenderCommand.h"

namespace Engine
{
	enum class RenderDebugView
	{
		Lit = 0, BaseColor, WorldNormal,
		Emissive, Alpha, CSM
	};

	struct RenderDebugSetting
	{
		RenderDebugView View = RenderDebugView::Lit;
		bool Wireframe = false;
	};

	class RenderDebugScope
	{
	public:
		explicit RenderDebugScope(std::string_view name)
		{
			RenderCommand::PushDebugGroup(name);
		}

		~RenderDebugScope()
		{
			RenderCommand::PopDebugGroup();
		}

		RenderDebugScope(const RenderDebugScope&) = delete;
		RenderDebugScope& operator=(const RenderDebugScope&) = delete;
	};
}