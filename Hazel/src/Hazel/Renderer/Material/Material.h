#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/RenderState.h"
#include "Hazel/Renderer/RHI/Shader.h"

namespace Engine
{
	struct MaterialRenderConfig
	{
		BlendMode Blend = BlendMode::Opaque;
		CullMode Cull = CullMode::Back;

		bool DepthTest = true;
		bool DepthWrite = true;
		bool CastShadow = true;
	};

	class Material
	{
	public:
		inline const Ref<Shader>& GetShader() const { return m_Shader; }
		inline const MaterialRenderConfig& GetRenderConfig() const { return m_Config; }

		inline void SetConfig(const MaterialRenderConfig& config) { m_Config = config; }
		inline bool SetShader(Ref<Shader>& shader) { 
			m_Shader = shader;
			return true;
		}
		
		static Ref<Material> Create(const Ref<Shader>& shader, MaterialRenderConfig config = {});


	private:
		explicit Material(const Ref<Shader>& shader, MaterialRenderConfig config)
			: m_Shader(shader), m_Config(config) { }
	private:
		Ref<Shader> m_Shader;
		MaterialRenderConfig m_Config;
	};

}
