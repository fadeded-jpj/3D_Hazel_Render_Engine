#pragma once

#include "Hazel/Renderer/RHI/Shader.h"

namespace Engine
{
	class ShaderManager
	{
	public:
		static void Init();
		static void Shutdown();

		// 懒加载，Get 时再加载
		static bool Register(const std::string& name, const AssetPath& sourcePath);

		static Ref<Shader> Get(const std::string& name);
		static bool Reload(const std::string& name);
	private:
		struct ShaderRecord
		{
			std::string name;
			AssetPath SourcePath;
			std::filesystem::path ResolvedPath;
			Ref<Shader> RuntimeShader;
			bool Dirty = true;
		};

		static std::unordered_map<std::string, ShaderRecord> s_Shaders;
	};
}