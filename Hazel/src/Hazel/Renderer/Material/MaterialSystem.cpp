#include "hzpch.h"
#include "Hazel/Renderer/Material/MaterialSystem.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "Hazel/Renderer/Resources/RenderResourceCache.h"
#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"

namespace Engine
{
	Ref<Material> MaterialSystem::GetImportedLitMaterial(const MaterialRenderConfig& config)
	{
		return GetImportedPBRMaterial(config);
	}
	Ref<Material> MaterialSystem::GetImportedPBRMaterial(const MaterialRenderConfig& config)
	{
		return GetOrCreatedMaterial("PBRForward", config);
	}
	Ref<Material> MaterialSystem::GetImportedToonMaterial(const MaterialRenderConfig& config)
	{
		return GetOrCreatedMaterial("ToonShader", config);
	}
	Ref<Material> MaterialSystem::GetOrCreatedMaterial(const std::string& shaderName, const MaterialRenderConfig& config)
	{
		auto shader = ShaderManager::Get(shaderName);
		if (!shader)
		{
			HZ_CORE_ERROR("Shader not available for material: {0}", shaderName.c_str());
			return nullptr;
		}
		return RenderResourceCache::GetMaterial(shader, config);
	}
	Ref<Material> MaterialSystem::GetDefaultMaterial(const MaterialRenderConfig& config)
	{
		// TODO: 后续根据config 的PBR/Toon 选择使用哪个默认材质
		return GetImportedLitMaterial(config);
	}
	Ref<Material> MaterialSystem::GetErrorMaterial(const MaterialRenderConfig& config)
	{
		if (auto material = GetOrCreatedMaterial("Error", config))
			return material;

		return GetDefaultMaterial(config);
	}
	ImportedToonMaterialDesc MaterialSystem::GetToonParameters(const Ref<MaterialInstance>& instance)
	{
		ImportedToonMaterialDesc toon;
		toon.Role = instance->GetToonMaterialRole();

		//instance->SetFloat("u_Threshold", toon.Threshold);
		//instance->SetFloat("u_ToonLitLevel", toon.LitLevel);
		//instance->SetFloat("u_ToonShadowLevel", toon.ShadowLevel);
		//instance->SetFloat("u_Softness", toon.Softness);
		//instance->SetFloat3("u_ToonShadowTint", toon.ShadowTint);

		const auto& Ints = instance->GetParameters().Ints;
		const auto& floats = instance->GetParameters().Floats;
		const auto& float3s = instance->GetParameters().Float3s;

		auto fn = [&](const std::string& name)->float
			{
				auto it = floats.find(name);
				if (it == floats.end())
					return 0.5f;
				return it->second;
			};

		toon.Threshold = fn("u_Threshold");
		toon.LitLevel = fn("u_ToonLitLevel");
		toon.ShadowLevel = fn("u_ToonShadowLevel");
		toon.Softness = fn("u_Softness");
		toon.RimIntensity = fn("u_RimIntensity");
		toon.RimPower = fn("u_RimPower");
		toon.RimLightMask = fn("u_RimLightMask");
		toon.FaceShadowSoftness = fn("u_FaceShadowSoftness");

		auto it0 = float3s.find("u_RimColor");
		if (it0 != float3s.end())
			toon.RimColor = it0->second;
		
		return toon;
	}
	Ref<MaterialInstance> MaterialSystem::CreateInstance(const Ref<Material>& material)
	{
		if (!material)
		{
			HZ_CORE_ERROR("Cannot create MaterialInstance from null material");
			return nullptr;
		}
		return MaterialInstance::Create(material);
	}
	bool MaterialSystem::SetImportedLitBlendMode(const Ref<MaterialInstance>& material, BlendMode blendMode)
	{
		if(!material)
			return false;

		auto overrideConfig = material->GetRenderConfig();
		overrideConfig.Blend = static_cast<Engine::BlendMode>(blendMode);

		switch (overrideConfig.Blend)
		{
		case Engine::BlendMode::Opaque:
			overrideConfig.DepthWrite = true;
			material->SetInt("u_AlphaMode", 0);
			break;

		case Engine::BlendMode::AlphaCutout:
			overrideConfig.DepthWrite = true;
			material->SetInt("u_AlphaMode", 1);
			break;

		case Engine::BlendMode::AlphaBlend:
			overrideConfig.DepthWrite = false;
			material->SetInt("u_AlphaMode", 0);
			break;
		}

		material->SetRenderConfigOverride(overrideConfig);
		return true;
	}

	bool MaterialSystem::SetImportedLitCullMode(const Ref<MaterialInstance>& material, CullMode cullmode)
	{
		if (!material)
			return false;

		auto overrideConfig = material->GetRenderConfig();
		overrideConfig.Cull = static_cast<Engine::CullMode>(cullmode);
		material->SetRenderConfigOverride(overrideConfig);
		return true;
	}
	bool MaterialSystem::SetImportedLitAlpha(const Ref<MaterialInstance>& material, float alpha)
	{
		if (!material)
			return false;
		material->SetBaseColorFactorAlpha(alpha);
		return true;
	}
	float MaterialSystem::GetImportedPBRSSRStrength(const Ref<MaterialInstance>& material)
	{
		if (!material)
			return 0.0f;

		const auto& floats = material->GetParameters().Floats;
		const auto it = floats.find("u_SSRStrength");
		return it != floats.end() ? it->second : 0.0f;
	}
	bool MaterialSystem::SetImportedPBRSSRStrength(const Ref<MaterialInstance>& material, float strength)
	{
		if (!material)
			return false;

		material->SetFloat("u_SSRStrength", std::clamp(strength, 0.0f, 1.0f));
		return true;
	}
	bool MaterialSystem::SetImportedToonParameters(const Ref<MaterialInstance>& material, ImportedToonMaterialDesc& toon)
	{
		if(!material)
			return false;

		material->SetToonMaterialRole(toon.Role);
		material->SetFloat("u_Threshold", toon.Threshold);
		material->SetFloat("u_ToonLitLevel", toon.LitLevel);
		material->SetFloat("u_ToonShadowLevel", toon.ShadowLevel);
		material->SetFloat("u_Softness", toon.Softness);
		material->SetFloat3("u_ToonShadowTint", toon.ShadowTint);

		material->SetFloat3("u_RimColor", toon.RimColor);
		material->SetFloat("u_RimIntensity", toon.RimIntensity);
		material->SetFloat("u_RimPower", toon.RimPower);
		material->SetFloat("u_RimLightMask", toon.RimLightMask);

		material->SetFloat("u_FaceShadowSoftness", toon.FaceShadowSoftness);

		return true;
	}
	bool MaterialSystem::SetImportedLitCastShadow(const Ref<MaterialInstance>& material, bool castShadow)
	{
		if (!material)
			return false;

		auto overrideConfig = material->GetRenderConfig();
		overrideConfig.CastShadow = castShadow;
		material->SetRenderConfigOverride(overrideConfig);
		return true;
	}
}
