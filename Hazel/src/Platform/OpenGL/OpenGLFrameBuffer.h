#pragma once

#include "Hazel/Renderer/RHI/FrameBuffer.h"
#include "OpenGLTexture.h"
namespace Engine
{
	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferSpecitification& spec);
		~OpenGLFrameBuffer();

		virtual void Bind() override;
		virtual void UnBind()  override;
		virtual void Resize(unsigned int width, unsigned int height) override;
		virtual Ref<Texture2D> GetColorAttachment(unsigned int index) override;
		virtual Ref<Texture2D> GetDepthAttachment() override;
		virtual unsigned int GetColorAttachmentCount() const override;
		virtual glm::ivec2 GetSize() override;
		virtual void AttachDepth(const Ref<Texture2D>& depth) override;
		virtual void AttachDepthCubeFace(const Ref<TextureCubeMap>& texture, unsigned int faceIndex = 0) override;
		virtual void AttachDepthArray(const Ref<Texture2DArray>& texture, unsigned int layerIndex) override;
		virtual void AttachColor(const Ref<Texture2D>& texture, unsigned int attachmentIndex = 0, unsigned int mipLevel = 0) override;
		virtual void SetDrawBuffers(const std::vector<unsigned int>& buffers) override;
		virtual void ClearColor(unsigned int index = 0, glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) override;

		inline const unsigned int GetWidth() const { return m_Width; }
		inline const unsigned int GetHeight() const { return m_Height; }
	private:
		void Invalidate();

		void GenColorAttachment(TextureFormat format);
		void GenDepthAttachment(TextureFormat format);

	private:
		FrameBufferSpecitification m_Spec;
		unsigned int m_Width, m_Height;
		unsigned int m_RenderID = 0;

		std::vector<Ref<OpenGLTexture2D>> m_ColorAttachments;
		Ref<OpenGLTexture2D> m_DepthAttachment;
	};
}
