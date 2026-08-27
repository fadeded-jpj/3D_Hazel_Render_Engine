#pragma once

#include "Hazel/Renderer/RHI/FrameBuffer.h"

namespace Engine
{
	struct constGBufferSpecitification
	{
		unsigned int Width = 1280;
		unsigned int Height = 720;
	};

	class GBuffer
	{
	public:
		virtual ~GBuffer() = default;

		void Bind();
		void UnBind();
		void Resize(unsigned int width, unsigned int height);

		Ref<Texture2D> GetAlbedo() const;
		Ref<Texture2D> GetNormalRoughness() const;
		Ref<Texture2D> GetEmissiveMetallic() const;
		Ref<Texture2D> GetDepth() const;
		Ref<Texture2D> GetToonParamter() const;
		Ref<Texture2D> GetRimParamter() const;
		void AttachTexture(const Ref<Texture2D>& target, unsigned int slot) const;
		void AttachDepth(const Ref<Texture2D>& target) const;
		glm::ivec2 GetSize() { return m_FrameBuffer->GetSize(); }

		static Ref<GBuffer> Create(constGBufferSpecitification& info);
	private:
		explicit GBuffer(constGBufferSpecitification& info);
	private:
		Ref<FrameBuffer> m_FrameBuffer;
	};
}