#pragma once

#include <cstdint>

#include "Hazel/Core/Core.h"

namespace Engine
{
	class MaterialInstance;
	class Shader;
	class ShaderParameters;

	class MaterialBinder
	{
	public:
		virtual ~MaterialBinder() = default;
		virtual void Bind(const Ref<MaterialInstance>& instance) = 0;
		virtual void Bind(const ShaderParameters& parameters, const Ref<Shader>& shader,
			uint32_t textureSlotBase) = 0;

		static Scope<MaterialBinder> Create();
	};
}
