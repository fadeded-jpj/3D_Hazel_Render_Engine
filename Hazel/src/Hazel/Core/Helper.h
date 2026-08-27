#pragma once

#include <optional>
#include <string>

namespace Engine
{
	template<typename Enum>
	inline std::optional<Enum> FromString(std::string str);
}