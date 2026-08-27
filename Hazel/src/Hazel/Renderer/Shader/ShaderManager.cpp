#include "hzpch.h"
#include "ShaderManager.h"

#include "Hazel/AssetsSystem/AssetFileSystem.h"

namespace Engine
{
	std::unordered_map<std::string, ShaderManager::ShaderRecord> ShaderManager::s_Shaders = {};

	void ShaderManager::Init()
	{
		s_Shaders.clear();
		
		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultShader.glsl"))
			Register("Default", *path);
		else
			HZ_CORE_ERROR("Invalid default shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultShader.glsl"))
			Register("Error", *path);
		else
			HZ_CORE_ERROR("Invalid Error shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultGBufferShader.glsl"))
			Register("GBuffer", *path);
		else
			HZ_CORE_ERROR("Invalid GBuffer shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultLightingPassShader.glsl"))
			Register("Lighting", *path);
		else
			HZ_CORE_ERROR("Invalid Lighting Pass shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/PBRLightingPassShader.glsl"))
			Register("PBRLighting", *path);
		else
			HZ_CORE_ERROR("Invalid PBR Lighting Pass shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/PBRForwardShader.glsl"))
			Register("PBRForward", *path);
		else
			HZ_CORE_ERROR("Invalid PBR Forward shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultToneMapShader.glsl"))
			Register("ToneMap", *path);
		else
			HZ_CORE_ERROR("Invalid Tone Mapping shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultShadowMapShader.glsl"))
			Register("ShadowMap", *path);
		else
			HZ_CORE_ERROR("Invalid ShadowMap shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/DefaultPointShadowMapShader.glsl"))
			Register("PointShadowMap", *path);
		else
			HZ_CORE_ERROR("Invalid ShadowMap shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/TestComputeShader.glsl"))
			Register("ComputeTest", *path);
		else
			HZ_CORE_ERROR("Invalid compute test shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonLightingPassShader.glsl"))
			Register("ToonLighting", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Lighting shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShader.glsl"))
			Register("ToonShader", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Lighting shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonCharacter.glsl"))
			Register("ToonCharacter", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Character shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonFace.glsl"))
			Register("ToonFace", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Face shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonEye.glsl"))
			Register("ToonEye", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Eye shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonEyeHightlight.glsl"))
			Register("ToonHighlight", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Eye Highlight shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonHair.glsl"))
			Register("ToonHair", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Hair shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ToonShaders/ToonMetal.glsl"))
			Register("ToonMetal", *path);
		else
			HZ_CORE_ERROR("Invalid Toon Metal shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/InvertedHullOutlineShader.glsl"))
			Register("InvertedHullOutline", *path);
		else
			HZ_CORE_ERROR("Invalid inverted hull outline shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/ScreenSpaceOutlineShader.glsl"))
			Register("ScreenSpaceOutline", *path);
		else
			HZ_CORE_ERROR("Invalid screen space outline shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/EdgeOutlineShader.glsl"))
			Register("GemotryOutline", *path);
		else
			HZ_CORE_ERROR("Invalid Gemotry outline shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/Bloom/BloomFilter.glsl"))
			Register("BloomFilter", *path);
		else
			HZ_CORE_ERROR("Invalid Bloom Filter shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/Bloom/DownSampling.glsl"))
			Register("BloomDown", *path);
		else
			HZ_CORE_ERROR("Invalid Bloom Down shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/Bloom/UpSampling.glsl"))
			Register("BloomUp", *path);
		else
			HZ_CORE_ERROR("Invalid Bloom Up shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/Anti-aliasing/TAA.glsl"))
			Register("TAA", *path);
		else
			HZ_CORE_ERROR("Invalid TAA shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/AO/SSAO.glsl"))
			Register("SSAO", *path);
		else
			HZ_CORE_ERROR("Invalid SSAO shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/AO/SSAOBlur.glsl"))
			Register("SSAOBlur", *path);
		else
			HZ_CORE_ERROR("Invalid SSAO blur shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/SSR/SSR.glsl"))
			Register("SSR", *path);
		else
			HZ_CORE_ERROR("Invalid SSR shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/SSR/HiZ.glsl"))
			Register("HiZ", *path);
		else
			HZ_CORE_ERROR("Invalid Hi-Z  shader path!");

		if (auto path = AssetPath::Parse("/Engine/Shaders/Common/ShadowMask.glsl"))
			Register("ShadowMask", *path);
		else
			HZ_CORE_ERROR("Invalid Shadow Mask shader path!");

	}

	void ShaderManager::Shutdown()
	{
		s_Shaders.clear();
	}

	bool ShaderManager::Register(const std::string& name, const AssetPath& sourcePath)
	{
		if (name.empty() || sourcePath.Empty())
			return false;
		auto phyPath = AssetFileSystem::Resolve(sourcePath);
		if (phyPath.empty())
		{
			HZ_CORE_ERROR("Failed to resolve shader path: {0}", sourcePath.String());
			return false;
		}

		if (!std::filesystem::is_regular_file(phyPath))
		{
			HZ_CORE_ERROR("Shader file does not exist: {0}", phyPath.string());
			return false;
		}

		if (s_Shaders.find(name) != s_Shaders.end())
		{
			HZ_CORE_WARN("Shader already registered: {0}", name);
			return false;
		}

		ShaderRecord record;
		record.name = name;
		record.SourcePath = sourcePath;
		record.ResolvedPath = phyPath;
		record.RuntimeShader = nullptr;
		record.Dirty = true;
		
		s_Shaders.emplace(name, std::move(record));
		return true;
	}

	Ref<Shader> ShaderManager::Get(const std::string& name)
	{
		auto it = s_Shaders.find(name);
		if (it == s_Shaders.end())
		{
			HZ_CORE_ERROR("Shader not registered: {0}", name);
			return nullptr;
		}

		auto& record = it->second;
		if (!record.RuntimeShader || record.Dirty)
		{
			record.ResolvedPath = AssetFileSystem::Resolve(record.SourcePath);
			if (!std::filesystem::is_regular_file(record.ResolvedPath))
			{
				HZ_CORE_ERROR("Shader file missing: {0}", record.ResolvedPath.string());
				return nullptr;
			}

			record.RuntimeShader = Shader::Create(record.ResolvedPath.string());
			record.Dirty = false;
		}
		return record.RuntimeShader;
	}
	bool ShaderManager::Reload(const std::string& name)
	{
		auto it = s_Shaders.find(name);
		if (it == s_Shaders.end())
		{
			HZ_CORE_ERROR("Shader not registered: {0}", name);
			return false;
		}

		auto& record = it->second;

		auto physicalPath = AssetFileSystem::Resolve(record.SourcePath);
		if (physicalPath.empty() || !std::filesystem::is_regular_file(physicalPath))
		{
			HZ_CORE_ERROR("Shader file missing: {0}", record.SourcePath.String());
			return false;
		}

		auto newShader = Shader::Create(physicalPath.string());

		if (!newShader)
		{
			HZ_CORE_ERROR("Failed to reload shader: {0}", name);
			return false;
		}

		record.ResolvedPath = physicalPath;
		record.RuntimeShader = newShader;
		record.Dirty = false;

		return true;
	}
}
