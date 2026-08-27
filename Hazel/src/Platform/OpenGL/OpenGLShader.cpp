#include "hzpch.h"
#include "OpenGLShader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <fstream>
#include <optional>
#include <filesystem>
#include <unordered_set>

#include "glm/gtc/type_ptr.hpp"

namespace Engine
{
	namespace
	{
		std::string Trim(const std::string& value)
		{
			const auto first = value.find_first_not_of(" \t");
			if (first == std::string::npos)
				return {};

			const auto last = value.find_last_not_of(" \t");
			return value.substr(first, last - first + 1);
		}

		std::optional<ShaderStage> ShaderStageFromString(const std::string& type)
		{
			if (type == "vertex")
				return ShaderStage::Vertex;
			if (type == "fragment" || type == "pixel")
				return ShaderStage::Fragment;
			if (type == "geometry")
				return ShaderStage::Geometry;
			if (type == "tess_control")
				return ShaderStage::TessellationControl;
			if (type == "tess_evaluation")
				return ShaderStage::TessellationEvaluation;
			if (type == "compute")
				return ShaderStage::Compute;
			return std::nullopt;
		}

		GLenum ToOpenGLShaderType(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::Vertex: return GL_VERTEX_SHADER;
			case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
			case ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
			case ShaderStage::TessellationControl: return GL_TESS_CONTROL_SHADER;
			case ShaderStage::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
			case ShaderStage::Compute: return GL_COMPUTE_SHADER;
			}

			HZ_CORE_ASSERT(false, "Unsupported shader stage");
			return 0;
		}

		ShaderProgramType ValidateAndGetProgramType(const ShaderStageSources& sources)
		{
			const bool hasCompute = sources.find(ShaderStage::Compute) != sources.end();
			if (hasCompute)
			{
				HZ_CORE_ASSERT(sources.size() == 1,
					"A compute program cannot contain graphics shader stages");
				return ShaderProgramType::Compute;
			}

			HZ_CORE_ASSERT(sources.find(ShaderStage::Vertex) != sources.end(),
				"A graphics shader requires a vertex stage");
			HZ_CORE_ASSERT(sources.find(ShaderStage::Fragment) != sources.end(),
				"A graphics shader requires a fragment stage");

			const bool hasTessellationControl = sources.find(ShaderStage::TessellationControl) != sources.end();
			const bool hasTessellationEvaluation = sources.find(ShaderStage::TessellationEvaluation) != sources.end();
			HZ_CORE_ASSERT(hasTessellationControl == hasTessellationEvaluation,
				"Tessellation control and evaluation stages must be provided together");

			return ShaderProgramType::Graphics;
		}
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragSrc)
		:m_Name(name)
	{
		m_UniformLocationCache.clear();
		ShaderStageSources sources = {
			{ ShaderStage::Vertex, vertexSrc },
			{ ShaderStage::Fragment, fragSrc }
		};
		Compile(sources);
	}

	OpenGLShader::OpenGLShader(const std::string& name, const ShaderStageSources& sources)
		: m_Name(name)
	{
		m_UniformLocationCache.clear();
		Compile(sources);
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		m_UniformLocationCache.clear();
		std::string shaderSrc = ReadFile(filepath);
		auto last = filepath.find_last_of("/\\");
		last = last == std::string::npos ? 0 : last + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - last : lastDot - last;
		m_Name = filepath.substr(last, count);

		auto stages = PreProcess(shaderSrc);
		std::error_code error;
		auto rootPath = std::filesystem::weakly_canonical(filepath, error);

		if (error)
			rootPath = std::filesystem::path(filepath).lexically_normal();

		for (auto& [stage, stageSource] : stages)
		{
			std::unordered_set<std::filesystem::path> includeStack;
			includeStack.insert(rootPath);

			stageSource = ExpandIncludes(stageSource, rootPath, includeStack);
		}

		Compile(stages);
	}
	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_ID);
	}
	void OpenGLShader::Bind() const
	{
		glUseProgram(m_ID);
	}
	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, const int& value)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, const float& value)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& vector)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniform2f(location, vector.x, vector.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& vector)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniform3f(location, vector.x, vector.y, vector.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& vector)
	{
		//glUseProgram(m_ID);
		const GLint location = GetUniformLocation(name);
		if (location == -1)
			return;
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
	}

	std::string OpenGLShader::ExpandIncludes(const std::string& origin, const std::filesystem::path& currentPath, std::unordered_set<std::filesystem::path>& includeStack)
	{
		// TODO: 在此处插入 return 语句
		std::istringstream input(origin);
		std::ostringstream output;
		std::string line;
		
		while (std::getline(input, line))
		{
			const std::string trimmedLine = Trim(line);

			if (trimmedLine.rfind("#include", 0) != 0)
			{
				output << line << '\n';
				continue;
			}

			const auto firstQuote = trimmedLine.find('"');
			const auto secondQuote = firstQuote == std::string::npos ? std::string::npos
				: trimmedLine.find('"', firstQuote + 1);;

			if (firstQuote == std::string::npos || secondQuote == std::string::npos)
			{
				HZ_CORE_ERROR("Invalid shader include directive in {0}: {1}", currentPath.string(), line.c_str());
				HZ_CORE_ASSERT(false, "Invalid shader include directive");
				return "";
			}

			const auto requestedPath = trimmedLine.substr(firstQuote + 1, secondQuote - firstQuote - 1);
			
			std::filesystem::path includePath = currentPath.parent_path() / requestedPath;
			std::error_code error;
			auto normalizedPath = std::filesystem::weakly_canonical(includePath, error);

			if (!error)
				includePath = normalizedPath;
			else
				includePath = includePath.lexically_normal();

			if (!std::filesystem::is_regular_file(includePath))
			{
				HZ_CORE_ERROR("Shader include file does not exist: {0}", includePath.string());

				HZ_CORE_ASSERT(false, "Shader include file does not exist");
				return {};
			}

			// 这里只检测当前递归链，允许 Vertex 和 Fragment 分别引用相同文件。
			if (!includeStack.insert(includePath).second)
			{
				HZ_CORE_ERROR(
					"Circular shader include detected: {0}",
					includePath.string());

				HZ_CORE_ASSERT(false, "Circular shader include");
				return {};
			}

			const auto includeSource = ReadFile(includePath.string());

			const auto expandSource = ExpandIncludes(includeSource, includePath, includeStack);

			output << expandSource;

			includeStack.erase(includePath);
			
		}

		return output.str();
	}

	void OpenGLShader::Compile(const ShaderStageSources& shaderSrcs)
	{
		HZ_CORE_ASSERT(!shaderSrcs.empty(), "Shader source cannot be empty");
		m_ProgramType = ValidateAndGetProgramType(shaderSrcs);

		GLuint program = glCreateProgram();
		std::vector<GLuint> shaderIDs;
		shaderIDs.reserve(shaderSrcs.size());

		for (const auto& [stage, source] : shaderSrcs)
		{
			const GLenum type = ToOpenGLShaderType(stage);
			GLuint shader = glCreateShader(type);
			const GLchar* sourceCStr = (const GLchar*)source.c_str();
			glShaderSource(shader, 1, &sourceCStr, 0);

			// Compile the vertex shader
			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

				glDeleteShader(shader);
				for (const auto id : shaderIDs)
					glDeleteShader(id);
				glDeleteProgram(program);

				HZ_CORE_ERROR("{0}", infoLog.data());
				HZ_CORE_ASSERT(false, "Shader compile failure!");
				return;
			}
			glAttachShader(program, shader);
			shaderIDs.push_back(shader);
		}

		m_ID = program;
		glLinkProgram(m_ID);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(m_ID, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_ID, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(m_ID, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(m_ID);
			m_ID = 0;
			
			for (const auto id : shaderIDs)
				glDeleteShader(id);

			HZ_CORE_ERROR("{0}", infoLog.data());
			HZ_CORE_ASSERT(false, "Shader link failure!");
			return;
		}

		for (const auto id : shaderIDs)
		{
			glDetachShader(m_ID, id);
			glDeleteShader(id);
		}
	}
	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		std::string result;
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else
		{
			HZ_CORE_WARN("Could not open file {0}", filepath);
		}
		return result;
	}
	ShaderStageSources OpenGLShader::PreProcess(const std::string& source)
	{
		ShaderStageSources shaderSrcs;
		const char* typeToken = "#type";
		const size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken);
		while (pos != std::string::npos)
		{
			const size_t eol = source.find_first_of("\r\n", pos);
			HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			const size_t begin = pos + typeTokenLength;
			const auto stage = ShaderStageFromString(Trim(source.substr(begin, eol - begin)));
			HZ_CORE_ASSERT(stage.has_value(), "Invalid shader stage specified");

			const size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			HZ_CORE_ASSERT(nextLinePos != std::string::npos, "Shader stage has no source");
			pos = source.find(typeToken, nextLinePos);
			const size_t sourceEnd = pos == std::string::npos ? source.size() : pos;
			const auto [_, inserted] = shaderSrcs.emplace(*stage, source.substr(nextLinePos, sourceEnd - nextLinePos));
			HZ_CORE_ASSERT(inserted, "Shader file contains a duplicated stage");
		}
		return shaderSrcs;
	}
	int OpenGLShader::GetUniformLocation(const std::string& name)
	{
		auto it = m_UniformLocationCache.find(name);
		if (it != m_UniformLocationCache.end())
			return it->second;

		const GLint location = glGetUniformLocation(m_ID, name.c_str());
		m_UniformLocationCache.emplace(name, location);

		return location;
	}
}
