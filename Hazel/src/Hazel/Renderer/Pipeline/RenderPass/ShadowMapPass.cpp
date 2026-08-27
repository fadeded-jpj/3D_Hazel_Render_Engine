#include "hzpch.h"
#include "ShadowMapPass.h"

#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"
#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"

namespace Engine
{
	bool ShadowMapPassBase::IsOpaqueLike(BlendMode blend)
	{
		return blend == BlendMode::Opaque || blend == BlendMode::AlphaCutout;
	}
	DrawItem ShadowMapPassBase::CreateDrawItem(const RenderObject& obj)
	{
		DrawItem item;
		item.Object = &obj;
		item.Mesh = obj.Mesh;
		item.Material = obj.Material;
		item.RenderConfig = item.Material->GetRenderConfig();
		item.ShaderProgram = item.Material->GetMaterial()->GetShader();

		return item;
	}

	ShadowMap2DPass::ShadowMap2DPass(unsigned int size, const Ref<Shader>& shader)
		: ShadowMapPassBase(shader)
	{
		m_ShadowMap2D = std::make_shared<ShadowMap>(size, size);
	}

	void ShadowMapPassBase::Build(const std::vector<RenderObject>& objects, std::function<bool(const RenderObject&)> func)
	{
		m_DrawItems.clear();
		m_DrawItems.reserve(objects.size());

		for (const auto& obj : objects)
		{
			if (func(obj))
				continue;

			m_DrawItems.push_back(CreateDrawItem(obj));
		}
	}

	RendererLog ShadowMap2DPass::Execute(const ShadowView& view, ShadowFrameData& output)
	{
		AppliedRenderState currentRenderState;
		RendererLog log;
		log.ShadowDrawCalls = static_cast<unsigned int>(m_DrawItems.size());

		m_ShadowMap2D->Bind();
		RenderCommand::Clear();

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetCull(CullMode::None);
		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetDepthTest(true);

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
		m_PerPassParameters.SetMat4("u_ViewProj", view.ViewProjection);
		m_PerPassParameters.SetFloat3("u_CameraPositon", view.ViewOrigin);
		const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
			m_PerPassParameters, m_Shader);

		for (const auto& item : m_DrawItems)
		{
			SubmitDrawItem(item, m_Shader, m_PerItemParameters, log, textureSlotOffset);
		}

		m_ShadowMap2D->UnBind();


		output.Enabled = true;
		output.ViewProjection = view.ViewProjection;
		output.DepthBias = view.DepthBias;

		return log;
	}


	void ShadowMapPassBase::SubmitDrawItem(const DrawItem& item, const Ref<Shader>& shader,
		ShaderParameters& parameters, RendererLog& log, uint32_t textureSlotOffset)
	{
		// parameters.Clear();
		const auto& obj = item.Object;

		if (obj->Skinning.BoneBuffer)
		{
			parameters.SetInt("u_HasSkeleton", 1);
			obj->Skinning.BoneBuffer->Bind(0);
		}
		else
		{
			parameters.SetInt("u_HasSkeleton", 0);
		}

		parameters.SetMat4("u_Model", obj->Transform);

		const bool needsAlphaTest =
			item.Material->GetRenderConfig().Blend == BlendMode::AlphaCutout;

		if (needsAlphaTest)
		{
			parameters.SetInt("u_AlphaMode", 1);
			textureSlotOffset = RenderCommand::ApplyMaterialParameters(
				item.Material, shader, textureSlotOffset);
		}
		else
		{
			// Opaque shadow draws only need the non-texture material defaults.
			parameters.SetInt("u_AlphaMode", 0);
		}

		// RenderCommand::ApplyMaterialParameters(item.Material, shader);
		RenderCommand::ApplyShaderParameters(parameters, shader, textureSlotOffset);
		RenderCommand::DrawMesh(item.Mesh);
		++log.DrawCalls;
	}

	void ShadowMapPassBase::ApplyRenderState(const MaterialRenderConfig& desired, AppliedRenderState& applied)
	{
		if (!applied.Initialized || applied.Blend != desired.Blend)
			RenderCommand::SetBlend(desired.Blend);
		if (!applied.Initialized || applied.Cull != desired.Cull)
			RenderCommand::SetCull(desired.Cull);
		if (!applied.Initialized || applied.DepthTest != desired.DepthTest)
			RenderCommand::SetDepthTest(desired.DepthTest);
		if (!applied.Initialized || applied.DepthWrite != desired.DepthWrite)
			RenderCommand::SetDepthWrite(desired.DepthWrite);

		applied.Initialized = true;
		applied.Blend = desired.Blend;
		applied.Cull = desired.Cull;
		applied.DepthTest = desired.DepthTest;
		applied.DepthWrite = desired.DepthWrite;
	}
	PointShadowPass::PointShadowPass(unsigned int size, const Ref<Shader>& shader)
		:ShadowMapPassBase(shader)
	{
		m_ShadowMap = std::make_shared<PointShadowMap>(size);
	}
	RendererLog PointShadowPass::Execute(const PointShadowView& view, ShadowFrameData& output)
	{
		AppliedRenderState currentRenderState;
		RendererLog log;
		log.ShadowDrawCalls = static_cast<unsigned int>(m_DrawItems.size());


		m_PerItemParameters.Clear();
		for (int i = 0; i < 6; i++)
		{
			m_ShadowMap->BindFace(i);
			RenderCommand::Clear();

			RenderCommand::SetBlend(BlendMode::Opaque);
			RenderCommand::SetCull(CullMode::None);
			RenderCommand::SetDepthWrite(true);
			RenderCommand::SetDepthTest(true);

			m_PerPassParameters.Clear();
			m_PerPassParameters.SetMat4("u_ViewProj", view.FaceViewProjection[i]);
			m_PerPassParameters.SetFloat3("u_CameraPositon", view.ViewOrigin);
			m_PerPassParameters.SetFloat3("u_LightPosition", view.ViewOrigin);
			m_PerPassParameters.SetFloat("u_FarPlane", view.FarPlane);
			const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
				m_PerPassParameters, m_Shader);

			for (const auto& item : m_DrawItems)
			{
				SubmitDrawItem(item, m_Shader, m_PerItemParameters, log, textureSlotOffset);
			}

			m_ShadowMap->UnBind();
		}

		output.Enabled = true;
		output.FarPlane = view.FarPlane;
		output.DepthBias = view.DepthBias;

		return log;
	}
	DirectionalCSMPass::DirectionalCSMPass(unsigned int size, unsigned int Layers, const Ref<Shader>& shader)
		:ShadowMapPassBase(shader)
	{
		m_ShadowMap = std::make_shared<CSMShadowMap>(size, size, Layers);

	}
	RendererLog DirectionalCSMPass::Execute(const CSMShadowView& view, ShadowFrameData& output)
	{
		AppliedRenderState currentRenderState;
		RendererLog log;
		log.ShadowDrawCalls = static_cast<unsigned int>(m_DrawItems.size()) * CSMShadowView::CascadeCount;
		

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetCull(CullMode::None);
		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetDepthTest(true);


		m_PerItemParameters.Clear();
		for (int i = 0; i < CSMShadowView::CascadeCount; i++)
		{
			m_ShadowMap->BindCascade(i);
			RenderCommand::Clear();

			m_PerPassParameters.Clear();
			m_PerPassParameters.SetMat4("u_ViewProj", view.ViewProjection[i]);
			const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
				m_PerPassParameters, m_Shader);

			for (const auto& item : m_DrawItems)
			{
				SubmitDrawItem(item, m_Shader, m_PerItemParameters, log, textureSlotOffset);
			}

			m_ShadowMap->UnBind();
		}

		output.Enabled = true;
		output.IsCSM = true;
		output.DepthBias = view.DepthBias;
		output.OverlapRatio = view.OverlapRatio;

		output.CascadeSplits = glm::vec4(
			view.SplitDepths[0],
			view.SplitDepths[1],
			view.SplitDepths[2],
			view.SplitDepths[3]
		);
		for (int i = 0; i < CSMShadowView::CascadeCount; i++)
			output.CSMViewProjection[i] = view.ViewProjection[i];

		return log;
	}
}
