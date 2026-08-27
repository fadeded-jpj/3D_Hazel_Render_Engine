#include "hzpch.h"
#include "OpenGLMaterial.h"

#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/RHI/Texture.h"
#include "OpenGLShader.h"
#include "Hazel/Renderer/Shader/ShaderParameters.h"
#include "Hazel/Renderer/Shader/ShaderTextureSlots.h"

namespace Engine
{
	OpenGLMaterialBinder::OpenGLMaterialBinder()
	{

	}
	OpenGLMaterialBinder::~OpenGLMaterialBinder()
	{
	}

	void OpenGLMaterialBinder::Bind(const Ref<MaterialInstance>& instance)
	{
		auto shader = dynamic_cast<OpenGLShader*>(instance->GetMaterial()->GetShader().get());
		shader->Bind();
		auto& parameter = instance->GetParameters();
		auto& textures = instance->GetTextures();
		UploadTextures(shader, textures, ShaderTextureSlots::First);
		UploadInts(shader, parameter);
		UploadFloats(shader, parameter);
		UploadFloat2s(shader, parameter);
		UploadFloat3s(shader, parameter);
		UploadFloat4s(shader, parameter);
		UploadMat3s(shader, parameter);
		UploadMat4s(shader, parameter);
	}

	void OpenGLMaterialBinder::Bind(const ShaderParameters& instance, const Ref<Shader>& inputshader,
		uint32_t textureSlotBase)
	{
		auto shader = dynamic_cast<OpenGLShader*>(inputshader.get());
		shader->Bind();
		auto& parameter = instance.GetUniforms();
		auto& textures = instance.GetTextures();
		UploadTextures(shader, textures, textureSlotBase);
		UploadInts(shader, parameter);
		UploadFloats(shader, parameter);
		UploadFloat2s(shader, parameter);
		UploadFloat3s(shader, parameter);
		UploadFloat4s(shader, parameter);
		UploadMat3s(shader, parameter);
		UploadMat4s(shader, parameter);
	}

	void OpenGLMaterialBinder::UploadTextures(OpenGLShader* shader, const MaterialTextureBlock& instance,
		uint32_t textureSlotBase) const
	{
		for (auto& [key, value] : instance)
		{
			const uint32_t slot = textureSlotBase + value.LocalSlot;
			value.Resource->Bind(slot);
			shader->UploadUniformInt(key, slot);
		}
	}
	void OpenGLMaterialBinder::UploadInts(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for(auto& [k, v] : instance.Ints)
			shader->UploadUniformInt(k, v);
	}
	void OpenGLMaterialBinder::UploadFloats(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Floats)
			shader->UploadUniformFloat(k, v);
	}
	void OpenGLMaterialBinder::UploadFloat2s(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Float2s)
			shader->UploadUniformFloat2(k, v);
	}
	void OpenGLMaterialBinder::UploadFloat3s(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Float3s)
			shader->UploadUniformFloat3(k, v);
	}
	void OpenGLMaterialBinder::UploadFloat4s(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Float4s)
			shader->UploadUniformFloat4(k, v);
	}
	void OpenGLMaterialBinder::UploadMat3s(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Mat3s)
			shader->UploadUniformMat3(k, v);
	}
	void OpenGLMaterialBinder::UploadMat4s(OpenGLShader* shader, const MaterialParameterBlock& instance) const
	{
		for (auto& [k, v] : instance.Mat4s)
			shader->UploadUniformMat4(k, v);
	}
}
