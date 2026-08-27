#include "hzpch.h"
#include "ShaderParameters.h"

namespace Engine
{
	void ShaderParameters::SetInt(const std::string& name, int value)
	{
		m_Uniforms.Ints[name] = value;
	}
	void ShaderParameters::SetFloat(const std::string& name, float value)
	{
		m_Uniforms.Floats[name] = value;

	}
	void ShaderParameters::SetFloat2(const std::string& name, glm::vec2 value)
	{
		m_Uniforms.Float2s[name] = value;
	}
	void ShaderParameters::SetFloat3(const std::string& name, glm::vec3 value)
	{
		m_Uniforms.Float3s[name] = value;
	}
	void ShaderParameters::SetFloat4(const std::string& name, glm::vec4 value)
	{
		m_Uniforms.Float4s[name] = value;
	}
	void ShaderParameters::SetMat3(const std::string& name, glm::mat3 mat)
	{
		m_Uniforms.Mat3s[name] = mat;
	}
	void ShaderParameters::SetMat4(const std::string& name, glm::mat4 mat)
	{
		m_Uniforms.Mat4s[name] = mat;
	}

	void ShaderParameters::SetTexture(const std::string& name, const Ref<Texture>& texture)
	{
		if (!texture)
			return;

		auto it = m_Textures.find(name);
		if (it != m_Textures.end())
		{
			it->second.Resource = texture;
			return;
		}

		const auto slot = AcquireTextureSlot();
		m_Textures[name] = { texture, slot };
	}

	void ShaderParameters::Clear()
	{
		m_Uniforms = {};
		m_Textures.clear();
	}

	unsigned int ShaderParameters::AcquireTextureSlot() const
	{
		for (unsigned int slot = 0;; slot++)
		{
			bool used = false;

			for (const auto& pair : m_Textures)
			{
				if (pair.second.LocalSlot == slot)
				{
					used = true;
					break;
				}
			}

			if (!used)
				return slot;
		}
	}
}
