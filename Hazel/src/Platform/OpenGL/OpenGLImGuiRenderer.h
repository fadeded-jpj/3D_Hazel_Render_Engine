#pragma once

#include "Hazel/ImGui/ImGuiRenderer.h"

namespace Engine
{
	class OpenGLImGuiRenderer : public ImGuiRenderer
	{
	public:
		static ImTextureID GetTextureID(const Ref<Texture2D>& texture);
	};
}