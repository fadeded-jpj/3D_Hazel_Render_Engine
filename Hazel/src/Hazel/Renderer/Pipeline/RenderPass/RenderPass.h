#pragma once

#include "Hazel/Renderer/Lighting/LightType.h"
#include "Hazel/Renderer/RenderTypes.h"
#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	struct ShadowFrameData;

	struct ShadowResourceSet;


	struct AppliedRenderState
	{
		bool Initialized = false;
		BlendMode Blend = BlendMode::Opaque;
		CullMode Cull = CullMode::Back;
		bool DepthTest = true;
		bool DepthWrite = true;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Build(const RenderView& view, const std::vector<RenderObject>& objects) = 0;
		virtual RendererLog Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow) = 0;
		const RenderPassItemStatistics& GetItemStatistics() const { return m_ItemStatistics; }

	protected:
		static DrawItem CreateDrawItem(const RenderObject& object, const glm::vec3& cameraPosition);
		static void SubmitPerItem(const DrawItem& item, const Ref<Shader>& shader,
			const RenderView& view, ShaderParameters& parameter, RendererLog& log,
			uint32_t textureSlotOffset);
		static void SubmitLight(ShaderParameters& parameters, const SceneLightingData& light,
			unsigned int maxLightCount = SceneLightingData::MAX_LIGHTS);
		static bool IsOpaqueLike(BlendMode blend);
		static bool IsTransparent(BlendMode blend);
		static void ApplyRenderState(const MaterialRenderConfig& desired, AppliedRenderState& applied);
		static bool CameraVisibleCull(const RenderObject& object, const glm::mat4& View);
		void ResetItemStatistics() { m_ItemStatistics = {}; }
		void RecordVisibilityResult(bool visible)
		{
			++m_ItemStatistics.InputItems;
			if (visible)
				++m_ItemStatistics.VisibleItems;
			else
				++m_ItemStatistics.CulledItems;
		}

	private:
		RenderPassItemStatistics m_ItemStatistics;
	};
}
