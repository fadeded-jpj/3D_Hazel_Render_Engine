#include "hzpch.h"
#include "MaterialInstance.h"

#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	Ref<MaterialInstance> MaterialInstance::Create(const Ref<Material>& material)
	{
		HZ_CORE_ASSERT(material, "MaterialInstance requires a Material");
		return Ref<MaterialInstance>(new MaterialInstance(material));
	}

	Ref<MaterialInstance> MaterialInstance::Clone()
	{
		auto res = Create(m_Material);
		res->m_RenderConfigOverride = m_RenderConfigOverride;
		res->m_Parameters = m_Parameters;
		res->m_ToonMaterialRole = m_ToonMaterialRole;
		return res;
	}

	void MaterialInstance::SetInt(const std::string& name, int value) { m_Parameters.SetInt(name, value); }
	void MaterialInstance::SetFloat(const std::string& name, float value) { m_Parameters.SetFloat(name, value); }
	void MaterialInstance::SetFloat2(const std::string& name, const glm::vec2& value) { m_Parameters.SetFloat2(name, value); }
	void MaterialInstance::SetFloat3(const std::string& name, const glm::vec3& value) { m_Parameters.SetFloat3(name, value); }
	void MaterialInstance::SetFloat4(const std::string& name, const glm::vec4& value) { m_Parameters.SetFloat4(name, value); }
	void MaterialInstance::SetMat3(const std::string& name, const glm::mat3& value) { m_Parameters.SetMat3(name, value); }
	void MaterialInstance::SetMat4(const std::string& name, const glm::mat4& value) { m_Parameters.SetMat4(name, value); }

	void MaterialInstance::SetTexture(const std::string& name, const Ref<Texture>& texture)
	{
		m_Parameters.SetTexture(name, texture);
	}
	void MaterialInstance::SetTextureCube(const std::string& name, const Ref<TextureCubeMap>& texture)
	{
		SetTexture(name, texture);
	}
}
