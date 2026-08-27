#include "hzpch.h"
#include "OpenGLFrameBuffer.h"


#include "glad/glad.h"

#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
	OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpecitification& spec)
		:m_Width(spec.Width), m_Height(spec.Height), m_Spec(spec)
	{
		Invalidate();
	}
	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_RenderID);
	}
	void OpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RenderID);
		glViewport(0, 0, m_Width, m_Height);
	}
	void OpenGLFrameBuffer::UnBind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void OpenGLFrameBuffer::Resize(unsigned int width, unsigned int height)
	{
		m_Width = width;
		m_Height = height;

		m_Spec.Width = width;
		m_Spec.Height = height;

		Invalidate();
	}
	Ref<Texture2D> OpenGLFrameBuffer::GetColorAttachment(unsigned int index)
	{
		return m_ColorAttachments[index];
	}
	Ref<Texture2D> OpenGLFrameBuffer::GetDepthAttachment()
	{
		return m_DepthAttachment;
	}
	unsigned int OpenGLFrameBuffer::GetColorAttachmentCount() const
	{
		HZ_CORE_ASSERT(m_ColorAttachments.size() <= std::numeric_limits<unsigned int>::max(),
			"Color attachment count exceeds the public framebuffer API range");
		return static_cast<unsigned int>(m_ColorAttachments.size());
	}
	glm::ivec2 OpenGLFrameBuffer::GetSize()
	{
		return { m_Width, m_Height };
	}
	void OpenGLFrameBuffer::AttachDepth(const Ref<Texture2D>& depth)
	{
		auto glDepth = std::dynamic_pointer_cast<OpenGLTexture2D>(depth);
		HZ_CORE_ASSERT(glDepth, "Depth attachment must be an OpenGLTexture2D");
		HZ_CORE_ASSERT(
			glDepth->GetWidth() == m_Width && glDepth->GetHeight() == m_Height,
			"Shared depth attachment size must match framebuffer size");

		m_DepthAttachment = glDepth;

		glNamedFramebufferTexture(
			m_RenderID,
			GL_DEPTH_STENCIL_ATTACHMENT,
			m_DepthAttachment->GetRenderID(),
			0);
	}
	void OpenGLFrameBuffer::AttachDepthCubeFace(const Ref<TextureCubeMap>& texture, unsigned int faceIndex)
	{
		HZ_CORE_ASSERT(faceIndex < 6, "Cubemap face index must be in [0, 5]");

		auto glCubeMap = std::dynamic_pointer_cast<OpenGLTextureCubeMap>(texture);
		HZ_CORE_ASSERT(glCubeMap, "Depth cubemap must be an OpenGLTextureCubeMap");

		glNamedFramebufferTextureLayer(
			m_RenderID,
			GL_DEPTH_ATTACHMENT,
			glCubeMap->GetRenderID(),
			0,
			static_cast<GLint>(faceIndex));

		glNamedFramebufferDrawBuffer(m_RenderID, GL_NONE);
		glNamedFramebufferReadBuffer(m_RenderID, GL_NONE);

		HZ_CORE_ASSERT(
			glCheckNamedFramebufferStatus(m_RenderID, GL_FRAMEBUFFER)
			== GL_FRAMEBUFFER_COMPLETE,
			"Point shadow framebuffer is incomplete after attaching cubemap face");
	}
	void OpenGLFrameBuffer::AttachDepthArray(const Ref<Texture2DArray>& texture, unsigned int layerIndex)
	{
		auto glTexture = std::dynamic_pointer_cast<OpenGLTexture2DArray>(texture);
		HZ_CORE_ASSERT(glTexture, "Depth array attachment must be an OpenGLTexture2DArray");
		HZ_CORE_ASSERT(layerIndex < glTexture->GetLayers(), "Layer index is out of bounds");
		HZ_CORE_ASSERT(glTexture->GetWidth() == m_Width && glTexture->GetHeight() == m_Height,
			"Shared depth attachment size must match framebuffer size");


		glNamedFramebufferTextureLayer(
			m_RenderID,
			GL_DEPTH_ATTACHMENT,
			glTexture->GetRenderID(),
			0,
			layerIndex);

		glNamedFramebufferDrawBuffer(m_RenderID, GL_NONE);
		glNamedFramebufferReadBuffer(m_RenderID, GL_NONE);

		HZ_CORE_ASSERT(
			glCheckNamedFramebufferStatus(m_RenderID, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
			"Framebuffer is incompelete!");
	}
	void OpenGLFrameBuffer::AttachColor(const Ref<Texture2D>& texture, unsigned int attachmentIndex, unsigned int mipLevel)
	{
		auto glTex = std::dynamic_pointer_cast<OpenGLTexture2D>(texture);
		HZ_CORE_ASSERT(glTex, "Color attachment must be an OpenGLTexture2D");

		HZ_CORE_ASSERT(mipLevel < glTex->GetMipLevelCount(), "Color attachment mip is out of range");

		const auto mipSize = glTex->GetMipSize(mipLevel);
		const GLenum drawBuffer = GL_COLOR_ATTACHMENT0 + attachmentIndex;
		if (m_ColorAttachments.size() <= attachmentIndex)
			m_ColorAttachments.resize(attachmentIndex + 1);
		m_ColorAttachments[attachmentIndex] = glTex;
		glNamedFramebufferTexture(m_RenderID, drawBuffer,
			glTex->GetRenderID(), static_cast<GLuint>(mipLevel));
		// glNamedFramebufferDrawBuffer(m_RenderID, drawBuffer);

		m_Width = mipSize.x;
		m_Height = mipSize.y;

		HZ_CORE_ASSERT(glCheckNamedFramebufferStatus(m_RenderID, GL_FRAMEBUFFER) ==
			GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete after attaching color mip");
	}
	void OpenGLFrameBuffer::SetDrawBuffers(const std::vector<unsigned int>& buffers)
	{
		if (buffers.size() == 0)
		{
			glNamedFramebufferDrawBuffer(m_RenderID, GL_NONE);
			glNamedFramebufferReadBuffer(m_RenderID, GL_NONE);
			return;
		}

		std::vector<GLenum> drawbuffers;
		for (auto& slot : buffers)
		{
			if (slot == FrameBuffer::UnusedAttachment)
				drawbuffers.push_back(GL_NONE);
			else
				drawbuffers.push_back(GL_COLOR_ATTACHMENT0 + slot);
		}

		HZ_CORE_ASSERT(drawbuffers.size() <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()),
			"Draw buffer count exceeds OpenGL GLsizei range");
		glNamedFramebufferDrawBuffers(
			m_RenderID,
			static_cast<GLsizei>(drawbuffers.size()),
			drawbuffers.data());
	}
	void OpenGLFrameBuffer::ClearColor(unsigned int index, glm::vec4 color)
	{
		glClearNamedFramebufferfv(m_RenderID, GL_COLOR, static_cast<GLint>(index), glm::value_ptr(color));
	}
	void OpenGLFrameBuffer::Invalidate()
	{
		if (m_RenderID)
		{
			glDeleteFramebuffers(1, &m_RenderID);
		}

		m_ColorAttachments.clear();
		m_DepthAttachment.reset();

		glCreateFramebuffers(1, &m_RenderID);
		Bind();

		// color attachment
		for (auto& spec : m_Spec.Attachments)
		{
			if (spec.format == TextureFormat::Depth24Stencil8)
				GenDepthAttachment(spec.format);
			else
				GenColorAttachment(spec.format);
		}

		std::vector<GLenum> drawBuffers;
		drawBuffers.reserve(m_ColorAttachments.size());

		for (uint32_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			drawBuffers.push_back(
				GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
		}

		if (!drawBuffers.empty())
		{
			glDrawBuffers(
				static_cast<GLsizei>(drawBuffers.size()),
				drawBuffers.data());
		}
		else
		{
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}

		const bool hasOwnedAttachment =
			!m_ColorAttachments.empty() || m_DepthAttachment != nullptr;

		if (hasOwnedAttachment)
		{
			HZ_CORE_ASSERT(
				glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Framebuffer Create failed!");
		}
		else
		{
			HZ_CORE_ASSERT(
				m_Spec.AllowEmptyAttachments,
				"Framebuffer has no attachments");
		}

		UnBind();
	}
	void OpenGLFrameBuffer::GenColorAttachment(TextureFormat format)
	{
		auto cur = std::make_shared<OpenGLTexture2D>(m_Width, m_Height, format);
		HZ_CORE_ASSERT(m_ColorAttachments.size() <= static_cast<size_t>(std::numeric_limits<GLenum>::max() - GL_COLOR_ATTACHMENT0),
			"Color attachment index exceeds OpenGL GLenum range");
		const GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(m_ColorAttachments.size());
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, cur->GetRenderID(), 0);
		m_ColorAttachments.push_back(std::move(cur));
	}
	void OpenGLFrameBuffer::GenDepthAttachment(TextureFormat format)
	{
		m_DepthAttachment = std::make_shared<OpenGLTexture2D>(m_Width, m_Height, format);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment->GetRenderID(), 0);

	}
}
