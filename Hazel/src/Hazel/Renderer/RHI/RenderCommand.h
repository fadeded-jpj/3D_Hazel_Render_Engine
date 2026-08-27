#pragma once

#include "RendererAPI.h"
#include "Hazel/Renderer/Material/MaterialBinder.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/RHI/Shader.h"
#include "Hazel/Renderer/Shader/ShaderParameters.h"
#include "Hazel/Renderer/Shader/ShaderTextureSlots.h"
namespace Engine
{

	class RenderCommand
	{
	public:
		inline static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }
		inline static void Clear() { s_RendererAPI->Clear(); } // Clear the current framebuffer.
		inline static void Shutdown()
		{
			s_MaterialBinder.reset();
		}

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { s_RendererAPI->SetViewport(x, y, width, height); }

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

		inline static void DrawMesh(const Ref<Mesh>& Mesh)
		{
			s_RendererAPI->DrawMesh(Mesh);
		}

		inline static void DispatchCompute(const Ref<Shader>& shader, uint32_t groupX, uint32_t groupY, uint32_t groupZ)
		{
			HZ_CORE_ASSERT(shader, "Compute dispatch requires a shader");
			HZ_CORE_ASSERT(shader->GetProgramType() == ShaderProgramType::Compute,
				"DispatchCompute requires a compute shader program");

			shader->Bind();
			s_RendererAPI->DispatchCompute(groupX, groupY, groupZ);
		}

		inline static void InsertMemoryBarrier(MemoryBarrierBits barriers)
		{
			s_RendererAPI->InsertMemoryBarrier(barriers);
		}

		inline static void Init() { s_RendererAPI->Init(); s_MaterialBinder = MaterialBinder::Create(); }

		inline static void BindMaterial(const Ref<MaterialInstance>& instance)
		{
			s_MaterialBinder->Bind(instance);
		}
		inline static uint32_t ApplyMaterialParameters(const Ref<MaterialInstance>& instance, const Ref<Shader>& shader,
			uint32_t textureSlotOffset)
		{
			const auto& parameters = instance->GetShaderParameters();
			s_MaterialBinder->Bind(parameters, shader, textureSlotOffset);
			return textureSlotOffset + static_cast<uint32_t>(parameters.GetTextures().size());
		}
		inline static uint32_t ApplyShaderParameters(const ShaderParameters& parameters, const Ref<Shader>& shader,
			uint32_t textureSlotOffset = ShaderTextureSlots::First)
		{
			s_MaterialBinder->Bind(parameters, shader, textureSlotOffset);
			return textureSlotOffset + static_cast<uint32_t>(parameters.GetTextures().size());
		}

		inline static void SetBlend(BlendMode mode) { s_RendererAPI->SetBlend(mode); }
		inline static void SetCull(CullMode mode) { s_RendererAPI->SetCull(mode); }
		inline static void SetDepthTest(bool enabled) { s_RendererAPI->SetDepthTest(enabled); }
		inline static void SetDepthWrite(bool enabled) { s_RendererAPI->SetDepthWrite(enabled); }
		inline static void SetPolygonMode(PolygonMode mode) { s_RendererAPI->SetPolygonMode(mode); }
		inline static void SetPolygonOffset(bool enabled, float x = 1.0f, float y = 2.0f) { 
			s_RendererAPI->SetPolygonOffset(enabled, x, y); 
		}

		inline static void PushDebugGroup(std::string_view name)
		{
			s_RendererAPI->PushDebugGroup(name);
		}

		inline static void PopDebugGroup()
		{
			s_RendererAPI->PopDebugGroup();
		}

		inline static uint32_t CreateTimestampQuery()
		{
			return s_RendererAPI->CreateTimestampQuery();
		}

		inline static void DeleteTimestampQuery(uint32_t query)
		{
			s_RendererAPI->DeleteTimestampQuery(query);
		}

		inline static void WriteTimestamp(uint32_t query)
		{
			s_RendererAPI->WriteTimestamp(query);
		}

		inline static bool IsTimestampAvailable(uint32_t query)
		{
			return s_RendererAPI->IsTimestampAvailable(query);
		}

		inline static uint64_t GetTimestamp(uint32_t query)
		{
			return s_RendererAPI->GetTimestamp(query);
		}

	private:
		static Scope<RendererAPI> s_RendererAPI;
		static Scope<MaterialBinder> s_MaterialBinder;
	};
}
