#include "hzpch.h"
#include "RenderPass.h"

#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"

namespace Engine
{
	DrawItem RenderPass::CreateDrawItem(const RenderObject& obj, const glm::vec3& cameraPosition)
	{
		DrawItem item;
		item.Object = &obj;
		item.Mesh = obj.Mesh;
		item.Material = obj.Material;
		item.RenderConfig = item.Material->GetRenderConfig();
		item.ShaderProgram = item.Material->GetMaterial()->GetShader();
		item.ToonRole = item.Material->GetToonMaterialRole();

		const glm::vec3 distance = cameraPosition - glm::vec3(obj.Transform[3]);
		item.CameraDistance = glm::dot(distance, distance);
		return item;
	}

	void RenderPass::SubmitPerItem(const DrawItem& item, const Ref<Shader>& shader,
			const RenderView& view, ShaderParameters& parameter, RendererLog& log,
			uint32_t textureSlotOffset)
	{
		const auto& obj = item.Object;
		const bool selected = view.SelectedID != INVALID_RENDER_OBJECT_ID && obj->ID == view.SelectedID;

		if (obj->Skinning.BoneBuffer)
		{
			parameter.SetInt("u_HasSkeleton", 1);
			obj->Skinning.BoneBuffer->Bind(0);

			const auto& previous =
				obj->Skinning.PreviousBoneBuffer
				? obj->Skinning.PreviousBoneBuffer
				: obj->Skinning.BoneBuffer;
			previous->Bind(1);
		}
		else
		{
			parameter.SetInt("u_HasSkeleton", 0);
		}

		parameter.SetMat4("u_Model", obj->Transform);
		parameter.SetInt("u_Selected", selected ? 1 : 0);
		textureSlotOffset = RenderCommand::ApplyMaterialParameters(
			item.Material, shader, textureSlotOffset);
		RenderCommand::ApplyShaderParameters(parameter, shader, textureSlotOffset);
		RenderCommand::DrawMesh(item.Mesh);
		++log.DrawCalls;
	}

	void RenderPass::SubmitLight(ShaderParameters& parameters, const SceneLightingData& light,
		unsigned int maxLightCount)
	{
		const unsigned int lightCount = std::min(light.LightCount, maxLightCount);
		parameters.SetInt("u_LightCount", static_cast<int>(lightCount));
		parameters.SetFloat3("u_AmbientColor", light.AmbientColor * light.AmbientIntensity);

		for (unsigned int i = 0; i < lightCount; ++i)
		{
			const auto& source = light.Lights[i];
			const auto prefix = "u_Lights[" + std::to_string(i) + "]";
			parameters.SetFloat4(prefix + ".ColorIntensity", source.ColorIntensity);
			parameters.SetFloat4(prefix + ".PositionRange", source.PositionRange);
			parameters.SetFloat4(prefix + ".DirectionType", source.DirectionType);
			parameters.SetFloat4(prefix + ".SpotAngles", source.SpotAngles);
		}
	}

	bool RenderPass::IsOpaqueLike(BlendMode blend)
	{
		return blend == BlendMode::Opaque || blend == BlendMode::AlphaCutout;
	}

	bool RenderPass::IsTransparent(BlendMode blend)
	{
		return blend == BlendMode::AlphaBlend;
	}

	void RenderPass::ApplyRenderState(const MaterialRenderConfig& desired, AppliedRenderState& applied)
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
	bool RenderPass::CameraVisibleCull(const RenderObject& object, const glm::mat4& ViewProj)
	{
		const auto& aabb = object.WorldBounds;

		const glm::vec4 points[] =
		{
			glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0),
			glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0),
			glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0),
			glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0),
			glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0),
			glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0),
			glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0),
			glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0)
		};

		bool outsideLeft = true;
		bool outsideRight = true;
		bool outsideBottom = true;
		bool outsideTop = true;
		bool outsideNear = true;
		bool outsideFar = true;

		for (const auto& point : points)
		{
			const glm::vec4 clip = ViewProj * point;
			outsideLeft &= clip.x < -clip.w;
			outsideRight &= clip.x > clip.w;
			outsideBottom &= clip.y < -clip.w;
			outsideTop &= clip.y > clip.w;
			outsideNear &= clip.z < -clip.w;
			outsideFar &= clip.z > clip.w;
		}

		return !(outsideLeft || outsideRight ||
			outsideBottom || outsideTop ||
			outsideNear || outsideFar);
	}
}
