#pragma once

#include <string>
#include <glm/glm.hpp>

#include <unordered_map>
#include <cstdint>

#include "Hazel/Core/Core.h"
#include "Hazel/AssetsSystem/AssetType.h"

namespace Engine
{
	enum class ShaderStage : uint8_t
	{
		Vertex,
		Fragment,
		Geometry,
		TessellationControl,
		TessellationEvaluation,
		Compute
	};

	enum class ShaderProgramType : uint8_t
	{
		Graphics,
		Compute
	};

	using ShaderStageSources = std::unordered_map<ShaderStage, std::string>;

	class Shader : public Asset
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual const std::string& GetName() const = 0;
		virtual ShaderProgramType GetProgramType() const = 0;
		
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragSrc);
		static Ref<Shader> Create(const std::string& name, const ShaderStageSources& sources);
		static Ref<Shader> Create(const std::string& filepath);

		ASSET_TYPE(Shader);

	private:

	};

	class ShaderLibrary
	{
	public:
		void Add(Ref<Shader>& shader);
		void Add(const std::string& name, Ref<Shader>& shader);
		Ref<Shader> Load(const std::string& filepath);
		Ref<Shader> Load(const std::string& name, const std::string& filepath);

		Ref<Shader> Get(const std::string& name);
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
		inline bool Exists(const std::string& name) const { return m_Shaders.find(name) != m_Shaders.end(); }
	};
}
