#pragma once

#include "Hazel/Renderer/Material/MaterialBinder.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "OpenGLShader.h"
namespace Engine
{
	class OpenGLMaterialBinder : public MaterialBinder
	{
	public:
		OpenGLMaterialBinder();
		virtual ~OpenGLMaterialBinder();

		virtual void Bind(const Ref<MaterialInstance>& instance) override;
		virtual void Bind(const ShaderParameters& instance, const Ref<Shader>& shader,
			uint32_t textureSlotBase) override;

	private:
		void UploadTextures(OpenGLShader* shader, const MaterialTextureBlock& instance,
			uint32_t textureSlotBase = 0) const;
		void UploadInts(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadFloats(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadFloat2s(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadFloat3s(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadFloat4s(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadMat3s(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
		void UploadMat4s(OpenGLShader* shader, const MaterialParameterBlock& instance) const;
	

	private:
		Ref<MaterialInstance> m_CurrentMaterial;
	};
}
