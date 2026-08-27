#pragma once

#include "Hazel/Renderer/Shader/ShaderParameters.h"
#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/Material/MaterialType.h"


namespace Engine
{

	class Texture;
	class TextureCubeMap;

	class MaterialInstance
	{
	public:
		const Ref<Material>& GetMaterial() const { return m_Material; }
		const MaterialParameterBlock& GetParameters() const { return m_Parameters.GetUniforms(); }
		const std::unordered_map<std::string, MaterialTextureBinding>& GetTextures() const { return m_Parameters.GetTextures(); }
		bool SetShader(Ref<Shader>& shader) { return m_Material->SetShader(shader); }
		const ShaderParameters& GetShaderParameters() const { return m_Parameters; }
		ToonMaterialRole GetToonMaterialRole() const { return m_ToonMaterialRole; }
		void SetToonMaterialRole(ToonMaterialRole role) { m_ToonMaterialRole = role; }

		void SetInt(const std::string& name, int value);
		void SetFloat(const std::string& name, float value);
		void SetFloat2(const std::string& name, const glm::vec2& value);
		void SetFloat3(const std::string& name, const glm::vec3& value);
		void SetFloat4(const std::string& name, const glm::vec4& value);
		void SetMat3(const std::string& name, const glm::mat3& value);
		void SetMat4(const std::string& name, const glm::mat4& value);
		void SetTexture(const std::string& name, const Ref<Texture>& texture);
		void SetTextureCube(const std::string& name, const Ref<TextureCubeMap>& texture);

		static Ref<MaterialInstance> Create(const Ref<Material>& material);
		Ref<MaterialInstance> Clone();

		inline const MaterialRenderConfig& GetRenderConfig() const {
			if (m_RenderConfigOverride)
				return *m_RenderConfigOverride;
			return m_Material->GetRenderConfig();
		}

		inline const MaterialRenderConfig& GetBaseRenderConfig() const
		{
			return m_Material->GetRenderConfig();
		}

		inline bool HasRenderConfigOverride() const
		{
			return m_RenderConfigOverride.has_value();
		}

		inline void SetRenderConfigOverride(const MaterialRenderConfig& config)
		{
			m_RenderConfigOverride = config;
		}

		inline void ResetRenderConfigOverride()
		{
			m_RenderConfigOverride.reset();
		}

		inline bool SetBaseColorFactorAlpha(float alpha)
		{
			auto it = m_Parameters.GetUniforms().Float4s.find("u_BaseColorFactor");
			if (it == m_Parameters.GetUniforms().Float4s.end())
				return false;

			it->second.a = alpha;
			return true;
		}

		inline glm::vec4 GetBaseColorFactor()
		{
			auto it = m_Parameters.GetUniforms().Float4s.find("u_BaseColorFactor");
			if (it != m_Parameters.GetUniforms().Float4s.end())
				return it->second;
			return glm::vec4(-1);
		}



	private:
		explicit MaterialInstance(const Ref<Material>& material)
			: m_Material(material) {}

		Ref<Material> m_Material;
		ShaderParameters m_Parameters;
		ToonMaterialRole m_ToonMaterialRole = ToonMaterialRole::Default;

		std::optional<MaterialRenderConfig> m_RenderConfigOverride;
	};
}
