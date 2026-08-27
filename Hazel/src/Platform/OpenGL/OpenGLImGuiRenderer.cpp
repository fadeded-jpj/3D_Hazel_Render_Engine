#include "hzpch.h"
#include "OpenGLImGuiRenderer.h"

#include "OpenGLTexture.h"

namespace Engine
{
	ImTextureID OpenGLImGuiRenderer::GetTextureID(const Ref<Texture2D>& texture)
	{
		return ImTextureID(std::dynamic_pointer_cast<OpenGLTexture2D>(texture)->GetRenderID());
	}
}