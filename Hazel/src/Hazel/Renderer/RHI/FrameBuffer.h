#pragma once

#include "Texture.h"
#include "glm/glm.hpp"

namespace Engine
{
	struct FrameBufferAttachmentSpecification
	{
		TextureFormat format;

		// 留给拓展BufferAttachement 参数用
	};

	struct FrameBufferSpecitification
	{
		unsigned int Width = 1280;
		unsigned int Height = 960;

		std::vector<FrameBufferAttachmentSpecification> Attachments = {
			{ TextureFormat::RGBA8 },
			{ TextureFormat::Depth24Stencil8}
		};

		bool AllowEmptyAttachments = false;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void Resize(unsigned int width, unsigned int height) = 0;

		virtual Ref<Texture2D> GetColorAttachment(unsigned int index) = 0;
		virtual Ref<Texture2D> GetDepthAttachment() = 0;
		virtual unsigned int GetColorAttachmentCount() const = 0;
		virtual glm::ivec2 GetSize() = 0;
		virtual void AttachDepth(const Ref<Texture2D>& depth) = 0;
		virtual void DetachDepth() = 0;
		virtual void AttachDepthCubeFace(const Ref<TextureCubeMap>& texture, unsigned int faceIndex = 0) = 0;
		virtual void AttachDepthArray(const Ref<Texture2DArray>& texture, unsigned int layerIndex) = 0;
		virtual void AttachColor(const Ref<Texture2D>& texture, unsigned int attachmentIndex = 0, unsigned int mipLevel = 0) = 0;
		virtual void SetDrawBuffers(const std::vector<unsigned int>& attachments) = 0;
		virtual void ClearColor(unsigned int index = 0, glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecitification& info);

		static constexpr uint32_t UnusedAttachment =
			std::numeric_limits<uint32_t>::max();
	};
}