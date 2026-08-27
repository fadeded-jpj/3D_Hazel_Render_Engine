#pragma once

#include "Hazel/Renderer/RHI/Shader.h"

namespace Engine
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragSrc);
		OpenGLShader(const std::string& name, const ShaderStageSources& sources);
		OpenGLShader(const std::string& filepath);
		~OpenGLShader();

		void Bind() const override;
		void Unbind() const override;
		inline const std::string& GetName() const override { return m_Name; } 
		inline ShaderProgramType GetProgramType() const override { return m_ProgramType; }

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);

		void UploadUniformInt(const std::string& name, const int& value);

		void UploadUniformFloat(const std::string& name, const float& value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& vector);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& vector);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& vector);


		// ShaderStage, std::string

	private:
		std::string ExpandIncludes(const std::string& shaderString, const std::filesystem::path& shaderPath,
			std::unordered_set<std::filesystem::path>& includeStack);
		void Compile(const ShaderStageSources& shaderSrcs);
		std::string ReadFile(const std::string& filepath);
		ShaderStageSources PreProcess(const std::string& source);

		int GetUniformLocation(const std::string& name);


	private:
		uint32_t m_ID = 0;
		std::string m_Name;
		ShaderProgramType m_ProgramType = ShaderProgramType::Graphics;

		std::unordered_map<std::string, int> m_UniformLocationCache;
	};
}
