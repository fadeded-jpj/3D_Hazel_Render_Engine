#pragma once

#include <cstdint>

#include "RenderPassProfile.h"

namespace Engine
{
	enum class RenderProfilePass : uint8_t
	{
		Frame = 0,
		SceneCSMShadow,
		SceneShadow2D,
		ScenePointShadow,
		CharacterShadow,
		GBuffer,
		SSAO,
		Lighting,
		ScreenSpaceOutline,
		InvertedHullOutline,
		Character,
		GeometryOutline,
		SSR,
		Transparent,
		TAA,
		Bloom,
		ToneMapping,
		Count
	};

	struct RenderProfile
	{
		RenderPassProfile Frame;
		RenderPassProfile SceneCSMShadow;
		RenderPassProfile SceneShadow2D;
		RenderPassProfile ScenePointShadow;
		RenderPassProfile CharacterShadow;
		RenderPassProfile GBuffer;
		RenderPassProfile SSAO;
		RenderPassProfile Lighting;
		RenderPassProfile ScreenSpaceOutline;
		RenderPassProfile InvertedHullOutline;
		RenderPassProfile Character;
		RenderPassProfile GeometryOutline;
		RenderPassProfile SSR;
		RenderPassProfile Transparent;
		RenderPassProfile TAA;
		RenderPassProfile Bloom;
		RenderPassProfile ToneMapping;
	};
}
