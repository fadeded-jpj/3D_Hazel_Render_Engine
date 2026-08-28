#include "hzpch.h"
#include "RenderPipeline.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"
#include "Hazel/Renderer/RenderDebug/RenderDebug.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "Hazel/Profile/RenderGPUProfiler.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	RenderPipeline::RenderPipeline()
	{
		m_CameraUniformBuffer = UniformBuffer::Create(
			sizeof(CameraUniformData),
			static_cast<uint32_t>(UniformBufferBinding::Camera));
	}

	void RenderPipeline::UpdateCameraUniformBuffer(const RenderView& view)
	{
		auto& target = m_CameraUniformBufferData;
		target.View = view.View;
		target.ViewProj = view.TAAEnabled
			? view.JitteredViewProjection
			: view.ViewProjection;
		target.InverseViewProj = view.TAAEnabled
			? view.InverseJitteredViewProjection
			: view.InverseViewProjection;

		target.Projection = target.ViewProj * glm::inverse(view.View);
		target.InverseProjection = view.View * target.InverseViewProj;

		target.PreViewProj = view.TAAEnabled
			? view.PreviousViewProjection
			: view.ViewProjection;

		target.JitteredViewProj = view.JitteredViewProjection;
		target.InverseJitteredViewProj = view.InverseJitteredViewProjection;

		target.StabledViewProj = view.ViewProjection;
		target.InverseStabledViewProj = view.InverseViewProjection;

		target.CameraPositionAndTime = glm::vec4(view.CameraPosition, view.Time);
		target.ViewportSizeAndJittered = glm::vec4(view.ViewportSize, view.JitterNDC);
		target.CameraClip = glm::vec4(
			view.NearClip, view.FarClip, view.PreviousJitterNDC);

		m_CameraUniformBuffer->SetData(
			&m_CameraUniformBufferData, sizeof(m_CameraUniformBufferData));
		m_CameraUniformBuffer->Bind(
			static_cast<uint32_t>(UniformBufferBinding::Camera));
	}

	ShadowView RenderPipeline::CaculateShadowView(const RenderView& view, const RenderLightResource& light)
	{
		LightType type = static_cast<LightType>(light.DirectionType.a);

		switch (type)
		{
		case LightType::Directional:
			return CaculateDirectionalShadowView(view, light);
		case LightType::Spot:
			return CaculateSpotShadowView(view, light);
		default:
			return ShadowView();
		}
	}

	ShadowView RenderPipeline::CaculateChacterShadowView(const RenderView& view, const RenderLightResource& light, const CharacterShadowBounds& bounds)
	{
		ShadowView res;
		res.Enabled = true;
		res.LightDirection = glm::normalize(glm::vec3(light.DirectionType.x, light.DirectionType.y, light.DirectionType.z));

		const auto& aabb = bounds.WorldAABB;
		const glm::vec3 PointsPos[8] = {
			{ aabb.Min.x, aabb.Min.y, aabb.Min.z },
			{ aabb.Max.x, aabb.Min.y, aabb.Min.z },
			{ aabb.Min.x, aabb.Max.y, aabb.Min.z },
			{ aabb.Max.x, aabb.Max.y, aabb.Min.z },
			{ aabb.Min.x, aabb.Min.y, aabb.Max.z },
			{ aabb.Max.x, aabb.Min.y, aabb.Max.z },
			{ aabb.Min.x, aabb.Max.y, aabb.Max.z },
			{ aabb.Max.x, aabb.Max.y, aabb.Max.z }
		};


		const glm::vec3 up =
			std::abs(glm::dot(res.LightDirection, glm::vec3(0, 1, 0))) > 0.99f
			? glm::vec3(0, 0, 1)
			: glm::vec3(0, 1, 0);


		// view
		// 这里view 只应用旋转
		// 平移的部分交给 projection
		// 不然 texel snapping 不好做
		// 另外还能适当减少计算开销
		auto View = glm::lookAt(glm::vec3(0), res.LightDirection, up);

		glm::vec3 aabbMax = glm::vec3(-FLT_MAX);
		glm::vec3 aabbMin = glm::vec3(FLT_MAX);

		for (int i = 0; i < 8; i++)
		{
			auto PointPosLightView = View * glm::vec4(PointsPos[i], 1.0f);
			aabbMax = glm::max(aabbMax, glm::vec3(PointPosLightView));
			aabbMin = glm::min(aabbMin, glm::vec3(PointPosLightView));
		}

		// Leave receiver and PCF margin around the character caster projection.
		constexpr float ShadowRangeScale = 2.0f;
		const glm::vec2 shadowCenter = (glm::vec2(aabbMin) + glm::vec2(aabbMax)) * 0.5f;
		glm::vec2 shadowHalfExtent = (glm::vec2(aabbMax) - glm::vec2(aabbMin)) * 0.5f;
		shadowHalfExtent = glm::max(shadowHalfExtent * ShadowRangeScale, glm::vec2(0.01f));
		aabbMin.x = shadowCenter.x - shadowHalfExtent.x;
		aabbMin.y = shadowCenter.y - shadowHalfExtent.y;
		aabbMax.x = shadowCenter.x + shadowHalfExtent.x;
		aabbMax.y = shadowCenter.y + shadowHalfExtent.y;

		float nearPlane = -aabbMax.z;
		float farPlane = nearPlane + view.FarClip;

		// 这里额外负责平移的部分
		// 只有对正交相机， proj 才可以负责平移
		auto Proj = glm::ortho(aabbMin.x, aabbMax.x,
			aabbMin.y, aabbMax.y, nearPlane, farPlane);

		res.ViewProjection = Proj * View;
		

		res.DepthBias = 0.00005f;

		return res;
	}

	ShadowView RenderPipeline::CaculateDirectionalShadowView(const RenderView& view, const RenderLightResource& light)
	{
		ShadowView res;
		res.Enabled = true;
		res.LightDirection = glm::normalize(glm::vec3(light.DirectionType.x, light.DirectionType.y, light.DirectionType.z));

		constexpr glm::vec3 focus = { 0.0f, 0.0f, 0.0f };
		constexpr float lightDistance = 100.0f;
		constexpr float halfExtent = 50.0f;

		const glm::vec3 up =
			std::abs(glm::dot(res.LightDirection, glm::vec3(0, 1, 0))) > 0.99f
			? glm::vec3(0, 0, 1)
			: glm::vec3(0, 1, 0);

		res.ViewOrigin = focus - lightDistance * res.LightDirection;
		res.View = glm::lookAt(res.ViewOrigin, focus, up);
		res.Projection = glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, 0.1f, lightDistance * 2.0f);

		res.ViewProjection = res.Projection * res.View;

		return res;
	}

	ShadowView RenderPipeline::CaculateSpotShadowView(const RenderView& view, const RenderLightResource& light)
	{
		ShadowView res;
		res.Enabled = true;
		res.LightDirection = glm::normalize(glm::vec3(light.DirectionType));
		res.ViewOrigin = glm::vec3(light.PositionRange);

		glm::vec3 focus = res.ViewOrigin + res.LightDirection;

		const glm::vec3 up =
			std::abs(glm::dot(res.LightDirection, glm::vec3(0, 1, 0))) > 0.99f
			? glm::vec3(0, 0, 1)
			: glm::vec3(0, 1, 0);

		res.View = glm::lookAt(res.ViewOrigin, focus, up);
		res.Projection = glm::perspective(glm::acos(light.SpotAngles.g) * 2.0f, 1.0f, 0.1f, light.PositionRange.a);

		res.ViewProjection = res.Projection * res.View;
		res.DepthBias = 0.0001f;

		return res;
	}

	PointShadowView RenderPipeline::CaculatePointShadowView(const RenderView& view, const RenderLightResource& light)
	{
		PointShadowView res;
		res.Enabled = true;
		res.ViewOrigin = glm::vec3(light.PositionRange);
		res.FarPlane = light.PositionRange.a;

		glm::mat proj = glm::perspective(glm::pi<float>() * 0.5f, 1.0f, res.NearPlane, res.FarPlane);

		const glm::vec3 directions[6] = {
			{ 1,  0,  0 }, { -1,  0,  0 },
			{ 0,  1,  0 }, {  0, -1,  0 },
			{ 0,  0,  1 }, {  0,  0, -1 }
		};

		const glm::vec3 ups[6] = {
			{ 0, -1,  0 }, { 0, -1,  0 },
			{ 0,  0,  1 }, { 0,  0, -1 },
			{ 0, -1,  0 }, { 0, -1,  0 }
		};

		for (int i = 0; i < 6; i++)
		{
			res.FaceViewProjection[i] = proj *
				glm::lookAt(res.ViewOrigin, res.ViewOrigin + directions[i], ups[i]);
		}
		res.DepthBias = 0.02f;
		return res;
	}

	CSMShadowView RenderPipeline::CaculateCSMShadowView(const RenderView& view, const RenderLightResource& light)
	{
		CSMShadowView res;
		res.Enabled = true;
		res.OverlapRatio = view.OverlapRatio;
		res.LightDirection = glm::normalize(glm::vec3(light.DirectionType.x, light.DirectionType.y, light.DirectionType.z));
		
		const auto& N = CSMShadowView::CascadeCount;
		const auto& nearClip = view.NearClip;
		const auto& farClip = std::min(view.FarClip, 50.0f);


		const glm::vec3 up =
			std::abs(glm::dot(res.LightDirection, glm::vec3(0, 1, 0))) > 0.99f
			? glm::vec3(0, 0, 1)
			: glm::vec3(0, 1, 0);

		auto split = [&](int i)->float
			{
				return  nearClip * glm::pow(farClip / nearClip, 
					static_cast<float>(i + 1) / static_cast<float>(N));
			};

		auto ViewDistance2NDC = [&](float distance)->float
			{
				float n = nearClip;
				float f = view.FarClip;

				const float a = (f + n) / (f - n);
				const float b = (2.0f * f * n) / (f - n);
				return a - b / distance;
			};

		// view
		// 这里view 只应用旋转
		// 平移的部分交给 projection
		// 不然 texel snapping 不好做
		// 另外还能适当减少计算开销
		auto View = glm::lookAt(glm::vec3(0), res.LightDirection, up);


		for (int i = 0; i < N; i++)
		{
			// 按对数分割每个级联的远近平面
			float l = i == 0 ? nearClip : split(i - 1);
			float r = split(i);
			res.SplitDepths[i] = r;

			// near 向前扩展做overlap
			if (i == 1)
				l = l - (res.SplitDepths[0] - nearClip) * res.OverlapRatio;
			else if (i > 1)
				l = l - (res.SplitDepths[i - 1] - res.SplitDepths[i - 2]) * res.OverlapRatio;


			// l 和 r 转到 NDC
			float zNearNDC = ViewDistance2NDC(l);
			float zFarNDC = ViewDistance2NDC(r);

			// 子视锥体 8 个顶点的NDC空间坐标
			glm::vec3 PointsPos[8] =
			{
				glm::vec3(-1, -1, zNearNDC) ,
				glm::vec3(-1, -1, zFarNDC)	,
				glm::vec3(-1,  1, zNearNDC)	,
				glm::vec3(-1,  1, zFarNDC)	,
				glm::vec3(1, -1, zNearNDC)	,
				glm::vec3(1, -1, zFarNDC)	,
				glm::vec3(1, 1, zNearNDC)	,
				glm::vec3(1, 1, zFarNDC)	,
			};

			// 把 NDC 空间坐标转换到世界空间
			// 同时计算子视锥体的AABB
			for (int i = 0; i < 8; i++)
			{
				glm::vec4 worldPos = view.InverseViewProjection * glm::vec4(PointsPos[i], 1.0f);
				PointsPos[i] = glm::vec3(worldPos) / worldPos.w;
			}
			
			// 投影的边界得使用 包围球
			// AABB 是轴对称的，经过旋转后可能会变成一个更大的AABB，所以使用包围球来计算正交投影的边界
			glm::vec3 focus = glm::vec3(0.0f);	// 球心
			for (auto& p : PointsPos)
				focus += p;
			focus /= 8.0f;

			float radius = 0.0f;
			for (const auto& p : PointsPos)
				radius = std::max(radius, glm::length(p - focus));
			
			const float lightDistance = 2.0f * radius;
			
			// texel snapping
			// light view 下的 focus 坐标
			glm::vec3 focusLS = glm::vec3(View * glm::vec4(focus, 1.0f));

			// 每像素对应的世界空间距离
			float texelSize = (2.0f * radius) / static_cast<float>(RenderView::ShadowMapResolution);

			// 计算 focusLS 对应的 像素坐标， 这里取整来保证 texel snapping
			focusLS.x = std::round(focusLS.x / texelSize) * texelSize;
			focusLS.y = std::round(focusLS.y / texelSize) * texelSize;


			// projection
			// 将子视锥体的8个顶点转换到光源空间，计算出最小和最大Z值
			float minZ = FLT_MAX;
			float maxZ = -FLT_MAX;
			for (auto& p : PointsPos)
			{
				glm::vec3 lightSpacePos = glm::vec3(View * glm::vec4(p, 1.0f));

				minZ = glm::min(minZ, lightSpacePos.z);
				maxZ = glm::max(maxZ, lightSpacePos.z);
			}

			maxZ += view.CSMCasterZOffset;

			// 这里额外负责平移的部分
			// 只有对正交相机， proj 才可以负责平移
			auto Proj = glm::ortho(focusLS.x - radius, focusLS.x + radius, 
				focusLS.y - radius, focusLS.y + radius, -maxZ, -minZ);

			//HZ_CORE_TRACE(
			//	"CSM {}: l={}, r={}, radius={}, minZ={}, maxZ={}",
			//	i, l, r, radius, minZ, maxZ);

			res.ViewProjection[i] = Proj * View;
		}

		res.DepthBias = 0.01f;

		return res;
	}


	ForwardRenderPipeline::ForwardRenderPipeline()
		:m_TonemappingPass(ShaderManager::Get("ToneMap")),
		m_ShadowMapPass(RenderView::ShadowMapResolution), m_PointShadowPass(RenderView::ShadowMapResolution)
	{
		FrameBufferSpecitification spec;
		spec.Attachments = {
			{ TextureFormat::RGBA16F },
			{ TextureFormat::Depth24Stencil8}
		};
		m_SceneColorBuffer = FrameBuffer::Create(spec);
	}
	RendererLog ForwardRenderPipeline::Render(const RenderView& view, Ref<FrameBuffer>& target,
		const std::vector<RenderObject>& objects, const SceneLightingData& light,
		const CharacterShadowBounds& characterBounds)
	{
		RendererLog log;
		view.JitterNDC = glm::vec2(0.0f);
		view.PreviousJitterNDC = glm::vec2(0.0f);
		view.JitteredViewProjection = view.ViewProjection;
		view.InverseJitteredViewProjection = view.InverseViewProjection;
		view.PreviousViewProjection = view.ViewProjection;
		view.PreviousJitteredViewProjection = view.ViewProjection;
		UpdateCameraUniformBuffer(view);

		ShadowFrameData shadow;

		auto IsFunc = [](const RenderObject& obj) {
			return !obj.Visible || obj.Domain == RenderDomain::Character
				|| obj.Material->GetRenderConfig().Blend == BlendMode::AlphaBlend
				|| !obj.Material->GetRenderConfig().CastShadow;
			};

		auto& mainLight = light.Lights[0];
		shadow.MainLightIndex = 0;
		shadow.lightPosition = mainLight.PositionRange;
		auto lightType = static_cast<LightType>(mainLight.DirectionType.a);
		// ---------- Shadow map 生成 -------------
		if (lightType == LightType::Directional || lightType == LightType::Spot)
		{
			auto shadowView = CaculateShadowView(view, mainLight);
			if (shadowView.Enabled)
			{
				m_ShadowMapPass.Build(objects, IsFunc);
				m_ShadowMapPass.Execute(shadowView, shadow);
			}
		}
		else if (lightType == LightType::Point)
		{
			auto shadowView = CaculatePointShadowView(view, mainLight);
			if (shadowView.Enabled)
			{
				m_PointShadowPass.Build(objects, IsFunc);
				m_PointShadowPass.Execute(shadowView, shadow);
			}
		}

		// 屏幕空间渲染
		m_SceneColorBuffer->Bind();
		RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		RenderCommand::Clear();

		RenderCommand::SetDepthTest(true);
		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetCull(CullMode::Back);

#ifdef HZ_DEBUG
		RenderCommand::SetPolygonMode(
			view.DebugSetting.Wireframe
			? PolygonMode::Line
			: PolygonMode::Fill);
#endif

		// ------------- 不透明Pass --------------

		m_OpaquePass.Build(view, objects);
		log += m_OpaquePass.Execute(view, light, shadow);


		// ------------- 透明 pass ---------------

		m_TransparentPass.Build(view, objects);
		log += m_TransparentPass.Execute(view, light, shadow);

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetCull(CullMode::Back);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetDepthWrite(true);

		RenderCommand::SetPolygonMode(PolygonMode::Fill);

		m_SceneColorBuffer->UnBind();

		target->Bind();
		// ---------- Tone Mapping ----------
		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetCull(CullMode::None);
		m_TonemappingPass.Execute(view, 
			m_SceneColorBuffer->GetColorAttachment(0), nullptr);

		target->UnBind();

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetCull(CullMode::Back);

		return log;
	}
	DeffedRenderPipline::DeffedRenderPipline()
		:m_GBufferPass(ShaderManager::Get("GBuffer")), m_GBuffer(GBuffer::Create(constGBufferSpecitification())),
		m_LightPass(ShaderManager::Get("PBRLighting")), m_TonemappingPass(ShaderManager::Get("ToneMap")),
		m_CSMShadowMapPass(RenderView::ShadowMapResolution), m_ShadowMapPass(RenderView::ShadowMapResolution),
		m_PointShadowPass(RenderView::ShadowMapResolution),
		m_InvertedHullOutlinePass(ShaderManager::Get("InvertedHullOutline")),
		m_ScreenSpaceOutlinePass(ShaderManager::Get("ScreenSpaceOutline")),
		m_CharacterShadowPass(RenderView::ShadowMapResolution),
		m_GemotryOutlinePass(ShaderManager::Get("GemotryOutline")),
		m_BloomPass(ShaderManager::Get("BloomFilter")),
		m_TAAPass(ShaderManager::Get("TAA")),
		m_SSAOPass(ShaderManager::Get("SSAO")),
		m_SSRPass(ShaderManager::Get("SSR"))
	{
		FrameBufferSpecitification spec;
		spec.Attachments = {
			{ TextureFormat::RGBA16F },			// SceneColor
			{ TextureFormat::RGBA16F},			// CharacterNormalMask
			{ TextureFormat::Depth24Stencil8}
		};
		m_SceneColorBuffer = FrameBuffer::Create(spec);

		m_ShadowResources.Scene2D = m_ShadowMapPass.GetDepth();
		m_ShadowResources.PointCube = m_PointShadowPass.GetDepth();
		m_ShadowResources.SceneCSM = m_CSMShadowMapPass.GetDepth();
		m_ShadowResources.Character2D = m_CharacterShadowPass.GetDepth();

		m_TAAFrameSources[0].Velocity = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RG16F);
		m_TAAFrameSources[0].Depth = Texture2D::Create(spec.Width, spec.Height, TextureFormat::Depth24Stencil8);
		m_TAAFrameSources[1].Velocity = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RG16F);
		m_TAAFrameSources[1].Depth = Texture2D::Create(spec.Width, spec.Height, TextureFormat::Depth24Stencil8);

	}

	void DeffedRenderPipline::PrepareCameraJitter(const RenderView& view)
	{
		const glm::vec2 jitterPixel = {
			Halton(m_TAAJitterFrameIndex + 1, 2) - 0.5f,
			Halton(m_TAAJitterFrameIndex + 1, 3) - 0.5f
		};

		view.JitterNDC = {
			2.0f * jitterPixel.x / view.ViewportSize.x,
			2.0f * jitterPixel.y / view.ViewportSize.y
		};

		auto jitterProjection = view.ViewProjection * glm::inverse(view.View);
		jitterProjection[2][0] -= view.JitterNDC.x;
		jitterProjection[2][1] -= view.JitterNDC.y;

		view.JitteredViewProjection = jitterProjection * view.View;
		view.InverseJitteredViewProjection = glm::inverse(view.JitteredViewProjection);

		++m_TAAJitterFrameIndex;
	}

	float DeffedRenderPipline::Halton(uint32_t index, uint32_t base)
	{
		float factor = 1.0f;
		float result = 0.0f;

		while (index > 0)
		{
			factor /= static_cast<float>(base);
			result += factor * static_cast<float>(index % base);
			index /= base;
		}

		return result;
	}

	RendererLog DeffedRenderPipline::Render(const RenderView& view, Ref<FrameBuffer>& target,
		const std::vector<RenderObject>& objects, const SceneLightingData& light,
		const CharacterShadowBounds& characterBounds)
	{
		RendererLog log;
		RenderGPUProfiler::BeginFrame(log.Profile);
		ShadowFrameData sceneShadow;
		ShadowFrameData characterShadow;

		const uint32_t currentIndex = m_TAASourceIndex;
		const uint32_t previousIndex = currentIndex ^ 1u;

		view.PreviousViewProjection = m_HasPreviousViewProjection
			? m_PreviousViewProjection
			: view.ViewProjection;

		// ------------- TAA Camera jitter -----------------------
		if (view.TAAEnabled)
		{
			PrepareCameraJitter(view);
			view.PreviousJitteredViewProjection = m_HasPreviousViewProjection
				? m_PreviousJitteredViewProjection
				: view.JitteredViewProjection;
		}
		else
		{
			view.JitterNDC = glm::vec2(0.0f);
			view.JitteredViewProjection = view.ViewProjection;
			view.InverseJitteredViewProjection = view.InverseViewProjection;
			view.PreviousJitteredViewProjection = view.PreviousViewProjection;
		}
		view.PreviousJitterNDC = m_HasPreviousViewProjection
			? m_PreviousJitterNDC
			: view.JitterNDC;
		// -----------------------------------------------------
		// -------------- 绑定UBO -------------------------
		UpdateCameraUniformBuffer(view);


		auto& mainLight = light.Lights[0];
		sceneShadow.MainLightIndex = 0;
		sceneShadow.lightPosition = glm::vec3(mainLight.PositionRange);
		characterShadow.MainLightIndex = 0;
		characterShadow.lightPosition = glm::vec3(mainLight.PositionRange);

		auto lightType = static_cast<LightType>(mainLight.DirectionType.a);

		auto IsEnvironment = [](const RenderObject& obj) {
			return !obj.Visible || obj.Domain == RenderDomain::Character
				|| obj.Material->GetRenderConfig().Blend == BlendMode::AlphaBlend
				|| !obj.Material->GetRenderConfig().CastShadow;
			};

		auto IsCharacter = [](const RenderObject& obj) {
			return !obj.Visible || obj.Domain != RenderDomain::Character
				|| obj.Material->GetRenderConfig().Blend == BlendMode::AlphaBlend
				|| !obj.Material->GetRenderConfig().CastShadow;
			};

		// ---------- Shadow map 生成 -------------
		// 环境阴影贴图
		if (view.CSMEnabled && lightType == LightType::Directional)
		{
			auto shadowView = CaculateCSMShadowView(view, mainLight);
			if (shadowView.Enabled)
			{
				RenderDebugScope debugScope("SceneCSMShadowPass");
				RenderPassCPUTimer cpuTimer(log.Profile.SceneCSMShadow);
				m_CSMShadowMapPass.Build(objects, IsEnvironment);
				RenderPassGPUTimer gpuTimer(RenderProfilePass::SceneCSMShadow);
				m_CSMShadowMapPass.Execute(shadowView, sceneShadow);
			}
		}
		else if (lightType == LightType::Directional || lightType == LightType::Spot)
		{
			auto shadowView = CaculateShadowView(view, mainLight);
			if (shadowView.Enabled)
			{
				RenderDebugScope debugScope("SceneShadow2DPass");
				RenderPassCPUTimer cpuTimer(log.Profile.SceneShadow2D);
				// 环境阴影贴图
				m_ShadowMapPass.Build(objects, IsEnvironment);
				RenderPassGPUTimer gpuTimer(RenderProfilePass::SceneShadow2D);
				m_ShadowMapPass.Execute(shadowView, sceneShadow);
			}
		}
		else if (lightType == LightType::Point)
		{
			auto shadowView = CaculatePointShadowView(view, mainLight);
			if (shadowView.Enabled)
			{
				RenderDebugScope debugScope("ScenePointShadowPass");
				RenderPassCPUTimer cpuTimer(log.Profile.ScenePointShadow);
				m_PointShadowPass.Build(objects, IsEnvironment);
				RenderPassGPUTimer gpuTimer(RenderProfilePass::ScenePointShadow);
				m_PointShadowPass.Execute(shadowView, sceneShadow);
			}
		}
		// Character-only shadow currently uses a 2D map, so point lights are excluded.
		if (lightType == LightType::Directional || lightType == LightType::Spot)
		{
			auto shadowView = CaculateChacterShadowView(view, mainLight, characterBounds);
			if (shadowView.Enabled)
			{
				RenderDebugScope debugScope("CharacterShadowPass");
				RenderPassCPUTimer cpuTimer(log.Profile.CharacterShadow);
				m_CharacterShadowPass.Build(objects, IsCharacter);
				RenderPassGPUTimer gpuTimer(RenderProfilePass::CharacterShadow);
				m_CharacterShadowPass.Execute(shadowView, characterShadow);
			}
		}

		// --------- GBuffer 生成 ---------------
		// ---------- 这里只生成 场景的 ----------
		// ---------- 人物 单独渲染 ------------
		// 需要管TAA
		const auto& size = target->GetSize();
		if (size != m_GBuffer->GetSize())
			m_GBuffer->Resize(size.x, size.y);
		if (m_TAAFrameSources[0].Depth->GetWidth() != size.x ||
			m_TAAFrameSources[0].Depth->GetHeight() != size.y)
		{
			m_TAAFrameSources[0].Velocity = Texture2D::Create(size.x, size.y, TextureFormat::RG16F);
			m_TAAFrameSources[0].Depth = Texture2D::Create(size.x, size.y, TextureFormat::Depth24Stencil8);
			m_TAAFrameSources[1].Velocity = Texture2D::Create(size.x, size.y, TextureFormat::RG16F);
			m_TAAFrameSources[1].Depth = Texture2D::Create(size.x, size.y, TextureFormat::Depth24Stencil8);
		}
		auto& currentSource = m_TAAFrameSources[currentIndex];
		auto& previousSource = m_TAAFrameSources[previousIndex];

		{
			RenderDebugScope debugScope("GBufferPass");
			RenderPassCPUTimer cpuTimer(log.Profile.GBuffer);
			m_GBufferPass.Build(view, objects);
			log.Profile.GBuffer.ItemStatistics = m_GBufferPass.GetItemStatistics();
			RenderPassGPUTimer gpuTimer(RenderProfilePass::GBuffer);
			m_GBuffer->Bind();

			// GBuffer 接入 velocity buffer
			// 已有 5 个输出， 故这里 slot = 5
			m_GBuffer->AttachTexture(currentSource.Velocity, 5);
			m_GBuffer->AttachDepth(currentSource.Depth);


			RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
			RenderCommand::Clear();

			RenderCommand::SetDepthTest(true);
			RenderCommand::SetDepthWrite(true);
			RenderCommand::SetCull(CullMode::Back);

#ifdef HZ_DEBUG
			RenderCommand::SetPolygonMode( view.DebugSetting.Wireframe
				? PolygonMode::Line : PolygonMode::Fill);
#endif 

			log += m_GBufferPass.Execute(view, light, {});

			m_GBuffer->UnBind();
		}

		// ---------------------- SSAO ----------------------
		{
			RenderDebugScope debugScope("SSAOPass");
			RenderPassCPUTimer cpuTimer(log.Profile.SSAO);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::SSAO);
			m_SSAOPass.Execute(view, m_GBuffer);
		}
		//---------------------------------------------------


		if (target->GetSize() != m_SceneColorBuffer->GetSize())
			m_SceneColorBuffer->Resize(target->GetSize().x, target->GetSize().y);
		{
			RenderDebugScope debugScope("DeferredLightingPass");
			RenderPassCPUTimer cpuTimer(log.Profile.Lighting);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::Lighting);
			m_SceneColorBuffer->Bind();
			m_SceneColorBuffer->ClearColor();
			m_SceneColorBuffer->DetachDepth();

#ifdef HZ_DEBUG
			RenderCommand::SetPolygonMode(PolygonMode::Fill);
#endif // HZ_DEBUG

			// --------- Light Pass ----------------
			// Projection 需要和 GBuffer 对齐
			RenderCommand::SetBlend(BlendMode::Opaque);
			RenderCommand::SetDepthTest(false);
			RenderCommand::SetDepthWrite(false);
			RenderCommand::SetCull(CullMode::None);
			m_ShadowResources.Bind();
			m_SceneColorBuffer->SetDrawBuffers({ 0 });
			m_LightPass.Execute(view, light, m_GBuffer, sceneShadow, characterShadow,
				m_SSAOPass.GetSSAOTexture(), m_SceneColorBuffer, m_ShadowResources.Character2D);
		}
		m_SceneColorBuffer->AttachDepth(m_GBuffer->GetDepth());
		

		// ------------- outline Pass --------------
		// backout 要
		// ScreenSpace outline
		//{
		//	RenderDebugScope debugScope("ScreenSpaceOutlinePass");
		//	RenderPassCPUTimer cpuTimer(log.Profile.ScreenSpaceOutline);
		//	RenderPassGPUTimer gpuTimer(RenderProfilePass::ScreenSpaceOutline);
		//	m_ScreenSpaceOutlinePass.Execute(view, m_GBuffer->GetNormalRoughness());

		//}
		{
			RenderDebugScope debugScope("InvertedHullOutlinePass");
			RenderPassCPUTimer cpuTimer(log.Profile.InvertedHullOutline);
			m_InvertedHullOutlinePass.Build(view, objects);
			log.Profile.InvertedHullOutline.ItemStatistics =
				m_InvertedHullOutlinePass.GetItemStatistics();
			RenderPassGPUTimer gpuTimer(RenderProfilePass::InvertedHullOutline);
			RenderCommand::SetDepthTest(true);
			RenderCommand::SetDepthWrite(false);
			RenderCommand::SetCull(CullMode::Front);
			RenderCommand::SetBlend(BlendMode::Opaque);
			log += m_InvertedHullOutlinePass.Execute(view, light, sceneShadow);
		}

		// ----------- character pass ----------
		// 要TAA
		{
			RenderDebugScope debugScope("CharacterPass");
			RenderPassCPUTimer cpuTimer(log.Profile.Character);
			m_CharacterPass.Build(view, objects);
			log.Profile.Character.ItemStatistics = m_CharacterPass.GetItemStatistics();
			RenderPassGPUTimer gpuTimer(RenderProfilePass::Character);
			RenderCommand::SetDepthTest(true);
			RenderCommand::SetDepthWrite(true);
			RenderCommand::SetCull(CullMode::Back);
#ifdef HZ_DEBUG
			RenderCommand::SetPolygonMode(view.DebugSetting.Wireframe
				? PolygonMode::Line : PolygonMode::Fill);
#endif 
			m_ShadowResources.Bind();
			m_SceneColorBuffer->AttachColor(currentSource.Velocity, 2);
			m_SceneColorBuffer->SetDrawBuffers({ 0, 1, 2 });
			m_SceneColorBuffer->ClearColor(1, glm::vec4(0.0f));	// 清除 character mask 的颜色缓存
			log += m_CharacterPass.Execute(view, light, sceneShadow, characterShadow);
			m_SceneColorBuffer->SetDrawBuffers({ 0 });
		}

		// 角色的内轮廓线
		{
			RenderDebugScope debugScope("GeometryOutlinePass");
			RenderPassCPUTimer cpuTimer(log.Profile.GeometryOutline);
			m_GemotryOutlinePass.Build(view, objects);
			log.Profile.GeometryOutline.ItemStatistics =
				m_GemotryOutlinePass.GetItemStatistics();
			RenderPassGPUTimer gpuTimer(RenderProfilePass::GeometryOutline);
			RenderCommand::SetDepthTest(true);
			RenderCommand::SetDepthWrite(false);
			RenderCommand::SetCull(CullMode::None);
			RenderCommand::SetBlend(BlendMode::AlphaBlend);
			log += m_GemotryOutlinePass.Execute(view, light, sceneShadow);
		}
		

#ifdef HZ_DEBUG
		RenderCommand::SetPolygonMode(PolygonMode::Fill);
#endif // HZ_DEBUG
		

		// ScreenSpace outline
		//{
		//	RenderDebugScope debugScope("ScreenSpaceOutlinePass");
		//	RenderPassCPUTimer cpuTimer(log.Profile.ScreenSpaceOutline);
		//	RenderPassGPUTimer gpuTimer(RenderProfilePass::ScreenSpaceOutline);
		//	m_ScreenSpaceOutlinePass.Execute(view, m_SceneColorBuffer->GetColorAttachment(1));

		//}

		// ----------- SSR Pass ---------------
		{
			RenderDebugScope debugScope("SSRPass");
			RenderPassCPUTimer cpuTimer(log.Profile.SSR);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::SSR);
			m_SSRPass.Execute(view, m_GBuffer, m_SceneColorBuffer);
		}
		// -------------------------------------------





		// ---------- Transparent Pass --------------
		// 先尝试一下 TAA
		{
			RenderDebugScope debugScope("TransparentPass");
			RenderPassCPUTimer cpuTimer(log.Profile.Transparent);
			m_TransparentPass.Build(view, objects);
			log.Profile.Transparent.ItemStatistics = m_TransparentPass.GetItemStatistics();
			RenderPassGPUTimer gpuTimer(RenderProfilePass::Transparent);
			m_SceneColorBuffer->SetDrawBuffers({ 0, 1, 2 });
			m_ShadowResources.Bind();
			log += m_TransparentPass.Execute(view, light, sceneShadow, characterShadow);
		}

		m_SceneColorBuffer->UnBind();
		

		// ------------ 后面是后处理了 开始做TAA ---------------------
		if(view.TAAEnabled)
		{
			RenderDebugScope debugScope("TAAPass");
			RenderPassCPUTimer cpuTimer(log.Profile.TAA);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::TAA);
			m_TAAPass.Execute(view, m_SceneColorBuffer,
				previousSource.Depth, previousSource.Velocity);
		}

		// ------------------ Bloom -------------------
		{
			RenderDebugScope debugScope("BloomPass");
			RenderPassCPUTimer cpuTimer(log.Profile.Bloom);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::Bloom);
			RenderCommand::SetBlend(BlendMode::Opaque);
			RenderCommand::SetDepthTest(false);
			RenderCommand::SetDepthWrite(false);
			RenderCommand::SetCull(CullMode::None);
			m_BloomPass.Execute(view,
				view.TAAEnabled ? m_TAAPass.GetHistory() :
				m_SceneColorBuffer->GetColorAttachment(0));
		}


		// ---------- Tone Mapping -------
		{
			RenderDebugScope debugScope("ToneMappingPass");
			RenderPassCPUTimer cpuTimer(log.Profile.ToneMapping);
			RenderPassGPUTimer gpuTimer(RenderProfilePass::ToneMapping);
			target->Bind();
			RenderCommand::SetBlend(BlendMode::Opaque);
			RenderCommand::SetDepthTest(false);
			RenderCommand::SetDepthWrite(false);
			RenderCommand::SetCull(CullMode::None);
			m_TonemappingPass.Execute(view, view.TAAEnabled ? m_TAAPass.GetHistory() :
				m_SceneColorBuffer->GetColorAttachment(0), m_BloomPass.GetBloomTexture());

			target->UnBind();
		}

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetCull(CullMode::Back);

		m_PreviousViewProjection = view.ViewProjection;
		m_PreviousJitteredViewProjection = view.JitteredViewProjection;
		m_PreviousJitterNDC = view.JitterNDC;
		m_HasPreviousViewProjection = true;
		log.OutlineCPUTime = log.Profile.InvertedHullOutline.CpuTimeMs;
		RenderGPUProfiler::EndFrame();

		m_TAASourceIndex = previousIndex;

		return log;
	}
}
