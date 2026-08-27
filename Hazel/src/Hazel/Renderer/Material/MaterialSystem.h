#pragma once
#include "Material.h"
#include "MaterialInstance.h"
namespace Engine
{
	struct ImportedToonMaterialDesc;

	class MaterialSystem
	{
	public:
		inline static void Init() {};
		inline static void Shutdown() {};

		static Ref<Material> GetImportedLitMaterial(const MaterialRenderConfig& config);
		static Ref<Material> GetImportedPBRMaterial(const MaterialRenderConfig& config);
		static Ref<Material> GetImportedToonMaterial(const MaterialRenderConfig& config);
		static Ref<Material> GetOrCreatedMaterial(const std::string& shaderName, const MaterialRenderConfig& config);
		static Ref<Material> GetDefaultMaterial(const MaterialRenderConfig& config = {});
		static Ref<Material> GetErrorMaterial(const MaterialRenderConfig& config);
		static ImportedToonMaterialDesc GetToonParameters(const Ref<MaterialInstance>& instance);

		static Ref<MaterialInstance> CreateInstance(const Ref<Material>& material);

		static bool SetImportedLitBlendMode(const Ref<MaterialInstance>& material, BlendMode blendMode);
		static bool SetImportedLitCullMode(const Ref<MaterialInstance>& material, CullMode cullmode);
		static bool SetImportedLitAlpha(const Ref<MaterialInstance>& material, float alpha);
		static float GetImportedPBRSSRStrength(const Ref<MaterialInstance>& material);
		static bool SetImportedPBRSSRStrength(const Ref<MaterialInstance>& material, float strength);
		static bool SetImportedToonParameters(const Ref<MaterialInstance>& material, ImportedToonMaterialDesc& toon);
		static bool SetImportedLitCastShadow(const Ref<MaterialInstance>& material, bool castShadow);

	private:

	};
}
