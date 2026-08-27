#pragma once

#include "Hazel/Renderer/Material/MaterialParameterBlock.h"

namespace Engine
{
	using MaterialTextureBlock = std::unordered_map<std::string, MaterialTextureBinding>;

	class ShaderParameters
	{
	public:
		void SetInt(const std::string& name, int value);
		void SetFloat(const std::string& name, float value);
		void SetFloat2(const std::string& name, glm::vec2 value);
		void SetFloat3(const std::string& name, glm::vec3 value);
		void SetFloat4(const std::string& name, glm::vec4 value);
		void SetMat3(const std::string& name, glm::mat3 mat);
		void SetMat4(const std::string& name, glm::mat4 mat);

		void SetTexture(const std::string& name, const Ref<Texture>& texture);
		void Clear();

		const MaterialParameterBlock& GetUniforms() const
		{
			return m_Uniforms;
		}
		MaterialParameterBlock& GetUniforms()
		{
			return m_Uniforms;
		}

		const MaterialTextureBlock& GetTextures() const
		{
			return m_Textures;
		}
	private:
		unsigned int AcquireTextureSlot() const;

	private:
		MaterialParameterBlock m_Uniforms;
		MaterialTextureBlock m_Textures;
	};
}
