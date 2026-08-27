#include "hzpch.h"
#include "GBuffer.h"

namespace Engine
{
	void GBuffer::Bind()
	{
		m_FrameBuffer->Bind();
	}
	void GBuffer::UnBind()
	{
		m_FrameBuffer->UnBind();
	}
	void GBuffer::Resize(unsigned int width, unsigned int height)
	{
		m_FrameBuffer->Resize(width, height);
	}

	Ref<Texture2D> GBuffer::GetAlbedo() const
	{
		return m_FrameBuffer->GetColorAttachment(0);
	}
	Ref<Texture2D> GBuffer::GetNormalRoughness() const
	{
		return m_FrameBuffer->GetColorAttachment(1);
	}
	Ref<Texture2D> GBuffer::GetEmissiveMetallic() const
	{
		return m_FrameBuffer->GetColorAttachment(2);
	}
	Ref<Texture2D> GBuffer::GetDepth() const
	{
		return m_FrameBuffer->GetDepthAttachment();
	}

	Ref<Texture2D> GBuffer::GetToonParamter() const
	{
		return m_FrameBuffer->GetColorAttachment(3);
	}
	Ref<Texture2D> GBuffer::GetRimParamter() const
	{
		return m_FrameBuffer->GetColorAttachment(4);
	}

	void GBuffer::AttachTexture(const Ref<Texture2D>& target, unsigned int slot) const
	{
		m_FrameBuffer->AttachColor(target, slot);
		m_FrameBuffer->SetDrawBuffers({ 0, 1, 2, FrameBuffer::UnusedAttachment, FrameBuffer::UnusedAttachment, 5 });
	}

	void GBuffer::AttachDepth(const Ref<Texture2D>& target) const
	{
		m_FrameBuffer->AttachDepth(target);
		m_FrameBuffer->SetDrawBuffers({ 0, 1, 2, FrameBuffer::UnusedAttachment, FrameBuffer::UnusedAttachment, 5 });
	}


	Ref<GBuffer> GBuffer::Create(constGBufferSpecitification& info)
	{
		return Ref<GBuffer>(new GBuffer(info));
	}

	GBuffer::GBuffer(constGBufferSpecitification& info)
	{
		m_FrameBuffer = FrameBuffer::Create({
				info.Width, info.Height,
				{
					{TextureFormat::RGBA8},				// base color + alpha
					{TextureFormat::RGBA16F},			// normal + roughness
					{TextureFormat::RGBA16F},			// emissive + metallic/AO
					{TextureFormat::RGBA16F},			// threshold, softness, litlevel, shadowlevel
					{TextureFormat::RGBA16F},			// rim color, rim power
					{TextureFormat::Depth24Stencil8}	// depth
				}
			});
	}
}