#pragma once

#include <cstdint>

namespace Engine::ShaderTextureSlots
{
	// Keep slot zero unused so an uninitialized sampler cannot alias a bound resource.
	constexpr uint32_t First = 1;
}
