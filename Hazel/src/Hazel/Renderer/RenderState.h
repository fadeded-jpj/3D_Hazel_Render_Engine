#pragma once

namespace Engine
{
	enum class BlendMode
	{
		Opaque, AlphaCutout, AlphaBlend, Add
	};

	enum class CullMode
	{
		None, Back, Front, Unset
	};

	enum class PolygonMode
	{
		Fill, Line
	};

	enum class RenderOutlineMode
	{
		None, InvertedHull, ScreenSpace
	};
}