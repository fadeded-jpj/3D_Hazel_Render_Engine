#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/RenderState.h"

#include <string_view>

namespace Engine
{
	enum class MemoryBarrierBits : uint32_t
	{
		None = 0,
		ShaderImageAccess = 1 << 0,
		TextureFetch = 1 << 1,
		ShaderStorage = 1 << 2,
		All = 0xFFFFFFFF
	};

	inline MemoryBarrierBits operator|(MemoryBarrierBits lhs, MemoryBarrierBits rhs)
	{
		return static_cast<MemoryBarrierBits>(
			static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
	}

	class Mesh;
	class VertexArray;

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, DirectX12 = 2
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
		virtual void DrawMesh(const Ref<Mesh>& Mesh) = 0;
		virtual void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;
		virtual void InsertMemoryBarrier(MemoryBarrierBits barriers) = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetBlend(BlendMode mode) = 0;
		virtual void SetCull(CullMode mode) = 0;
		virtual void SetDepthTest(bool flag) = 0;
		virtual void SetDepthWrite(bool flag) = 0;
		virtual void SetPolygonMode(PolygonMode mode) = 0;
		virtual void SetPolygonOffset(bool enabled, float x, float y) = 0;

		virtual void PushDebugGroup(std::string_view name) = 0;
		virtual void PopDebugGroup() = 0;

		virtual uint32_t CreateTimestampQuery() = 0;
		virtual void DeleteTimestampQuery(uint32_t query) = 0;
		virtual void WriteTimestamp(uint32_t query) = 0;
		virtual bool IsTimestampAvailable(uint32_t query) = 0;
		virtual uint64_t GetTimestamp(uint32_t query) = 0;

		inline static API GetAPI() { return s_API; }
	private:
		static API s_API;
	};

}


