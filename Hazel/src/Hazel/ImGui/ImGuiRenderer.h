#pragma once

#include "imgui.h"

#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	class ImGuiRenderer
	{
	public:
		static ImTextureID GetTextureID(const Ref<Texture2D>& texture);
	};
}
