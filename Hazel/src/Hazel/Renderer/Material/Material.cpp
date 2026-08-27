#include "hzpch.h"
#include "Material.h"

#include "Hazel/Renderer/RHI/Shader.h"

namespace Engine
{
	Ref<Material> Material::Create(const Ref<Shader>& shader, MaterialRenderConfig config)
	{
		HZ_CORE_ASSERT(shader, "Material requires a shader");
		return Ref<Material>(new Material(shader, config));
	}
}
