#pragma once

#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	struct OpenGLTextureFormatInfo;

	TextureFormat SelectFileFormat(int channels, TextureColorSpace colorSpace);
	OpenGLTextureFormatInfo GetOpenGLFormat(TextureFormat format);

	class OpenGLTexture2D : public Texture2D
	{
	public:
		//OpenGLTexture2D(const std::string& path);
		OpenGLTexture2D(const std::filesystem::path& path, const TextureLoadOptions& options = {});
		OpenGLTexture2D(int width, int height, TextureFormat format,
			uint32_t miplevels = 1, TextureWrap wrap = TextureWrap::ClampToEdge);
		virtual ~OpenGLTexture2D();

		inline virtual uint32_t GetWidth() const override { return m_Width; }
		inline virtual uint32_t GetHeight() const override { return m_Height; }
		inline virtual uint32_t GetMipLevelCount() const override { return m_MipLevels; }
		virtual void Bind(unsigned int slot = 0) const override;
		virtual Ref<Texture2D> Clone() override;
		virtual void CopyFrom(const Ref<Texture2D>& source) override;

		inline unsigned int GetRenderID() { return m_RenderID; }

	private:
		std::filesystem::path m_Path;
		unsigned int m_RenderID = 0;
		unsigned int m_Width = 0, m_Height = 0;
		unsigned int m_MipLevels = 1;
		TextureFormat m_Format = TextureFormat::None;
		TextureWrap m_TextureWrap = TextureWrap::ClampToEdge;
	private:
		// void BindData(unsigned char* data, int& w, int& h, int& c);
		void Allocate(unsigned int width, unsigned int height, TextureFormat format,
			unsigned int mipLevels, TextureWrap wrap);
		void Upload(const void* data, unsigned int mipLevels);
		
	};


	class OpenGLTextureCubeMap : public TextureCubeMap
	{
	public:
		OpenGLTextureCubeMap(unsigned int size, TextureFormat format);
		OpenGLTextureCubeMap(const std::filesystem::path& path, const TextureLoadOptions& options);
		virtual ~OpenGLTextureCubeMap();

		virtual uint32_t GetSize() const override { return m_Size; }
		virtual void Bind(unsigned int slot = 0) const override;

		inline unsigned int GetRenderID() const { return m_RenderID; }
		inline virtual uint32_t GetMipLevelCount() const override { return m_MipLevels; }

	private:


	private:
		unsigned int m_RenderID = 0;
		unsigned int m_Size = 0;
		unsigned int m_MipLevels = 1;
		TextureFormat m_Format = TextureFormat::None;
	};

	class OpenGLTexture2DArray : public Texture2DArray
	{
	public:
		OpenGLTexture2DArray(unsigned int width, unsigned int height, unsigned int count, TextureFormat format);
		virtual ~OpenGLTexture2DArray();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual void Bind(unsigned int slot = 0) const override;

		inline unsigned int GetRenderID() const { return m_RenderID; }
		inline unsigned int GetLayers() const { return m_Count; }

		inline virtual uint32_t GetMipLevelCount() const override { return m_MipLevels; }

	private:
		unsigned int m_RenderID = 0;
		unsigned int m_Width = 0;
		unsigned int m_Height = 0;
		unsigned int m_Count = 0;
		unsigned int m_MipLevels = 1;
		TextureFormat m_Format = TextureFormat::None;
	};
}
