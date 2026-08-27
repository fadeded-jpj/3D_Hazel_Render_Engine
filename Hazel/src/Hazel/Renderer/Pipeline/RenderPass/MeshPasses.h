#pragma once

#include "RenderPass.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "Hazel/Renderer/Shader/ShaderParameters.h"
#include "Hazel/AssetsSystem/AssetManager.h"

#include "Hazel/Renderer/RHI/Texture.h"

#include "Hazel/Renderer/Material/MaterialType.h"

namespace Engine
{
	class OpaquePass : public RenderPass
	{
	public:
		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;

	private:
		void Sort();

	private:
		std::vector<DrawItem> m_DrawItems;
		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};

	class TransparentPass : public RenderPass
	{
	public:
		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& sceneShadow, const ShadowFrameData& characterShadow);

	private:
		void Sort();

	private:
		std::vector<DrawItem> m_DrawItems;

		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};

	class GBufferPass : public RenderPass
	{
	public:
		explicit GBufferPass(const Ref<Shader>& shader);

		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;

	private:
		void Sort();

	private:
		std::vector<DrawItem> m_DrawItems;
		Ref<Shader> m_GBufferShader;
		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};

	class InvertedHullOutlinePass : public RenderPass
	{
	public:
		InvertedHullOutlinePass(const Ref<Shader>& shader);

		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;
	private:
		// void SubmitPerItem(const DrawItem& item, const Ref<Shader>& shader, const RenderView& view, RendererLog& log);
	private:
		std::vector<DrawItem> m_DrawItems;
		Ref<Shader> m_OutlineShader;
		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};

	class CharacterPass : public RenderPass
	{
	public:
		CharacterPass(const Ref<Shader>& shader = ShaderManager::Get("ToonCharacter"))
		{
			auto path = AssetPath::Parse("/Game/models/Feiying/Feiying_FaceShadowMap_Anime.png");
			m_FaceShadowMap = AssetManager::GetAsset<Texture2D>(*path);

			path = AssetPath::Parse("/Game/models/Feiying/Metal_Mask.png");
			m_MetalMask = AssetManager::GetAsset<Texture2D>(*path);

			m_ToonShaders[ToonMaterialRole::Default]		= shader;
			m_ToonShaders[ToonMaterialRole::Face]			= ShaderManager::Get("ToonFace");
			m_ToonShaders[ToonMaterialRole::Eye]			= ShaderManager::Get("ToonEye");
			m_ToonShaders[ToonMaterialRole::EyeHighlight]	= ShaderManager::Get("ToonHighlight");
			m_ToonShaders[ToonMaterialRole::Hair]			= shader;
			m_ToonShaders[ToonMaterialRole::Skin]			= shader;
			m_ToonShaders[ToonMaterialRole::Metal]			= ShaderManager::Get("ToonMetal");
		}
		void SetToonShader(ToonMaterialRole role, const Ref<Shader>& shader);
		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& sceneShadow, const ShadowFrameData& characterShadow);

	private:
		void Sort();
		Ref<Shader> ResolveToonShader(ToonMaterialRole role) const;

	private:
		std::vector<DrawItem> m_DrawItems;
		Ref<Texture2D> m_FaceShadowMap;
		Ref<Texture2D> m_MetalMask;

		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;

		std::unordered_map<ToonMaterialRole, Ref<Shader>> m_ToonShaders;
	};

	class GemotryOutlinePass : public RenderPass
	{
	public:
		GemotryOutlinePass(const Ref<Shader>& shader);

		void Build(const RenderView& view, const std::vector<RenderObject>& objects) override;
		RendererLog Execute(const RenderView& view, const SceneLightingData& light,
			const ShadowFrameData& shadow) override;

	private:
		std::vector<DrawItem> m_DrawItems;
		Ref<Shader> m_OutlineShader;

		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};
}
