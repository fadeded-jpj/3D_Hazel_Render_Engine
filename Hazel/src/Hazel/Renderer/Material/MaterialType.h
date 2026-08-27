#pragma once

#include <cstdint>

namespace Engine
{
	enum class MaterialImportMode : uint8_t
	{
		Auto,
		PBR,
		Toon
	};

	enum class ToonMaterialRole : uint8_t
	{
		Default, Face, Eye, EyeHighlight,
		Hair, Skin, Metal,
		Count
	};
}
