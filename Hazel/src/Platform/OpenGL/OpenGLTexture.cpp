#include "hzpch.h"
#include "OpenGLTexture.h"

#include "stb_image.h"
#include <glad/glad.h>

namespace Engine
{
    struct OpenGLTextureFormatInfo
    {
        GLenum InternalFormat = 0;
        GLenum DataFormat = 0;
        GLenum DataType = 0;
    };

	namespace
	{
		GLenum ToOpenGLTextureWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::Repeat:
				return GL_REPEAT;
			case TextureWrap::ClampToEdge:
				return GL_CLAMP_TO_EDGE;
			}

			HZ_CORE_ASSERT(false, "Unsupported texture wrap mode");
			return GL_REPEAT;
		}
	}

  //  OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
  //      :m_Path(path)
  //  {
  //      int w, h, channels;
  //      stbi_set_flip_vertically_on_load(1);
  //      auto data = stbi_load(path.c_str(), &w, &h, &channels, 0);
		//HZ_CORE_ASSERT(data, "Failed to load image: {0} !", path);
		//BindData(data, w, h, channels);
  //  }

    OpenGLTexture2D::OpenGLTexture2D(const std::filesystem::path& path, const TextureLoadOptions& options)
        : m_Path(path)
    {
        int w, h, channels;
        stbi_set_flip_vertically_on_load(options.FlipVertically ? 1 : 0);

		FILE* file = _wfopen(path.c_str(), L"rb");
        HZ_CORE_ASSERT(file, "Failed to open image: {0} !", path.string());
		auto data = stbi_load_from_file(file, &w, &h, &channels, 0);
		fclose(file);
		HZ_CORE_ASSERT(data, "Failed to load image data!");

        const TextureFormat format = SelectFileFormat(channels, options.ColorSpace);
        const UINT miplevels = options.GenerateMips ?  1u + static_cast<uint32_t>(
            std::floor(std::log2(
                std::max(w, h))))
            : 1u;
        Allocate(w, h, format, miplevels, TextureWrap::Repeat);
        Upload(data, 0);

        if (options.GenerateMips)
            glGenerateTextureMipmap(m_RenderID);

        stbi_image_free(data);
    }

    OpenGLTexture2D::OpenGLTexture2D(int width, int height, TextureFormat format,
		uint32_t miplevels, TextureWrap wrap)
    {
        Allocate(width, height, format, miplevels, wrap);
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
		glDeleteTextures(1, &m_RenderID);
    }


    void OpenGLTexture2D::Bind(unsigned int slot) const
    {
        glBindTextureUnit(slot, m_RenderID);
    }
    Ref<Texture2D> OpenGLTexture2D::Clone()
    {
        Ref<OpenGLTexture2D> clone = std::make_shared<OpenGLTexture2D>(
            m_Width, m_Height, m_Format, m_MipLevels, m_TextureWrap
        );

		for (uint32_t mip = 0; mip < m_MipLevels; ++mip)
        {
            const GLsizei width =
                static_cast<GLsizei>(std::max(1u, m_Width >> mip));
            const GLsizei height =
                static_cast<GLsizei>(std::max(1u, m_Height >> mip));

            glCopyImageSubData(m_RenderID, GL_TEXTURE_2D, mip, 0, 0, 0,
                clone->GetRenderID(), GL_TEXTURE_2D, mip, 0, 0, 0,
                width, height, 1);
        }

        return clone;
    }
    void OpenGLTexture2D::CopyFrom(const Ref<Texture2D>& source)
    {
        auto glTex = std::dynamic_pointer_cast<OpenGLTexture2D>(source);
        HZ_CORE_ASSERT(glTex, "Invalid source texture");
        HZ_CORE_ASSERT(glTex->GetWidth() == m_Width &&
            glTex->GetHeight() == m_Height,
            "Texture sizes must match");
        glCopyImageSubData(glTex->GetRenderID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            m_RenderID, GL_TEXTURE_2D, 0, 0, 0, 0, m_Width, m_Height, 1);
    }
    //void OpenGLTexture2D::BindData(unsigned char* data, int& w, int& h, int& channels)
    //{
    //    
    //    m_Width = w;
    //    m_Height = h;

    //    GLenum internalFormat = 0, dataFormat = 0;
    //    if (channels == 4)
    //    {
    //        internalFormat = GL_RGBA8;
    //        dataFormat = GL_RGBA;
    //    }
    //    else if (channels == 3)
    //    {
    //        internalFormat = GL_RGB8;
    //        dataFormat = GL_RGB;
    //    }

    //    HZ_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

    //    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //    glCreateTextures(GL_TEXTURE_2D, 1, &m_RenderID);
    //    glTextureStorage2D(m_RenderID, 1, internalFormat, m_Width, m_Height);

    //    glTextureParameteri(m_RenderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //    glTextureParameteri(m_RenderID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //    if(data)
    //        glTextureSubImage2D(m_RenderID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

    //    stbi_image_free(data);
    //}
    void OpenGLTexture2D::Allocate(unsigned int width, unsigned int height, TextureFormat format,
		unsigned int mipLevels, TextureWrap wrap)
    {
        HZ_CORE_ASSERT(format != TextureFormat::None, "Allocated Texture!");
        HZ_CORE_ASSERT(width > 0 && height > 0, "Allocated Texture!");
        HZ_CORE_ASSERT(mipLevels > 0, "Allocated Texture!");

        m_Width = width;
        m_Height = height;
        m_Format = format;
        m_MipLevels = mipLevels;
        m_TextureWrap = wrap;

        const auto info = GetOpenGLFormat(format);

        if (m_RenderID != 0)
            glDeleteTextures(1, &m_RenderID);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RenderID);

        glTextureStorage2D(
            m_RenderID,
            static_cast<GLsizei>(mipLevels),
            info.InternalFormat,
            static_cast<GLsizei>(width),
            static_cast<GLsizei>(height));

        if(format == TextureFormat::Depth24Stencil8)
            glTextureParameteri(
                m_RenderID,
                GL_DEPTH_STENCIL_TEXTURE_MODE,
                GL_DEPTH_COMPONENT);

        glTextureParameteri(
            m_RenderID,
            GL_TEXTURE_MIN_FILTER,
            mipLevels > 1
            ? GL_LINEAR_MIPMAP_LINEAR
            : GL_LINEAR);

        glTextureParameteri(
            m_RenderID,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);

        const GLenum glWrap = ToOpenGLTextureWrap(wrap);

        glTextureParameteri(
            m_RenderID,
            GL_TEXTURE_WRAP_S,
            glWrap);

        glTextureParameteri(
            m_RenderID,
            GL_TEXTURE_WRAP_T,
            glWrap);
    }
    void OpenGLTexture2D::Upload(const void* data, unsigned int mipLevels)
    {
        HZ_CORE_ASSERT(m_RenderID != 0, "Texture haven't been allocated!");
        HZ_CORE_ASSERT(data, "Texture haven't been allocated");
        HZ_CORE_ASSERT(mipLevels < m_MipLevels, "Texture haven't been allocated");

        const auto info = GetOpenGLFormat(m_Format);

        const uint32_t mipWidth =
            std::max(1u, m_Width >> mipLevels);

        const uint32_t mipHeight =
            std::max(1u, m_Height >> mipLevels);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTextureSubImage2D(
            m_RenderID,
            static_cast<GLint>(mipLevels),
            0,
            0,
            static_cast<GLsizei>(mipWidth),
            static_cast<GLsizei>(mipHeight),
            info.DataFormat,
            info.DataType,
            data);
    }
    TextureFormat SelectFileFormat(int channels, TextureColorSpace colorSpace)
    {
        switch (channels)
        {
        case 1:
            return TextureFormat::R8;

        case 2:
            return TextureFormat::RG8;

        case 3:
            return colorSpace == TextureColorSpace::SRGB
                ? TextureFormat::RGB8_SRGB
                : TextureFormat::RGB8;

        case 4:
            return colorSpace == TextureColorSpace::SRGB
                ? TextureFormat::RGBA8_SRGB
                : TextureFormat::RGBA8;
        }

        return TextureFormat::None;
    }

    OpenGLTextureFormatInfo GetOpenGLFormat(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8:
            return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };

        case TextureFormat::RG8:
            return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };

        case TextureFormat::RG16F:
            return { GL_RG16F, GL_RG, GL_FLOAT };

        case TextureFormat::RGB8:
            return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };

        case TextureFormat::RGBA8:
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };

        case TextureFormat::RGB8_SRGB:
            return { GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE };

        case TextureFormat::RGBA8_SRGB:
            return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE };

        case TextureFormat::RGBA16F:
            return { GL_RGBA16F, GL_RGBA, GL_FLOAT };

        case TextureFormat::R16F:
            return { GL_R16F, GL_RED, GL_FLOAT };

        case TextureFormat::R32F:
            return { GL_R32F, GL_RED, GL_FLOAT };

        case TextureFormat::Depth24Stencil8:
            return {
                GL_DEPTH24_STENCIL8,
                GL_DEPTH_STENCIL,
                GL_UNSIGNED_INT_24_8
            };
        case TextureFormat::Depth32F:
            return{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT };
        }

        HZ_CORE_ASSERT(false, "Unsupported texture format");
        return {};
    }




    OpenGLTextureCubeMap::OpenGLTextureCubeMap(unsigned int size, TextureFormat format)
        : m_Format(format), m_Size(size)
    {  
        HZ_CORE_ASSERT(format != TextureFormat::None, "Allocated Texture!");
        HZ_CORE_ASSERT(size > 0 , "Allocated Texture!");
      
        const auto info = GetOpenGLFormat(m_Format);

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RenderID);
        glTextureStorage2D(m_RenderID, 1, info.InternalFormat, size, size);


        glTextureParameteri(m_RenderID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_RenderID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    }
    OpenGLTextureCubeMap::OpenGLTextureCubeMap(const std::filesystem::path& path, const TextureLoadOptions& options)
    {
        HZ_CORE_ASSERT(!options.FlipVertically, "Cubemap cross texture must not be flipped vertically");

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(false);

        FILE* file = _wfopen(path.c_str(), L"rb");
        HZ_CORE_ASSERT(file, "Failed to open cubemap image");

        stbi_uc* source = stbi_load_from_file(file, &width, &height, &channels, 0);
        fclose(file);

        HZ_CORE_ASSERT(source, "Failed to load cubemap image");
        HZ_CORE_ASSERT(
            width % 4 == 0 && height % 3 == 0,
            "Cubemap cross must use a 4x3 layout");

        const int faceWidth = width / 4;
        const int faceHeight = height / 3;

        HZ_CORE_ASSERT(faceWidth == faceHeight, "Cubemap faces must be square");

        m_Size = faceWidth;
        m_Format = SelectFileFormat(channels, options.ColorSpace);

        m_MipLevels = options.GenerateMips ?
            Texture::CalculateMipLevelCount(m_Size, m_Size) : 1;

        const auto formaInfo = GetOpenGLFormat(m_Format);

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RenderID);

        glTextureStorage2D(m_RenderID, m_MipLevels, formaInfo.InternalFormat, m_Size, m_Size);

        constexpr glm::ivec2 faceRegions[6] =
        {
            { 2, 1 }, // +X
            { 0, 1 }, // -X
            { 1, 0 }, // +Y
            { 1, 2 }, // -Y
            { 1, 1 }, // +Z
            { 3, 1 }  // -Z 
        };

        const auto faceRowBytes =
            static_cast<size_t>(faceWidth) * static_cast<size_t>(channels);

        std::vector<stbi_uc> facePixels(
            static_cast<size_t>(faceWidth) *
            static_cast<size_t>(faceHeight) *
            static_cast<size_t>(channels));

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (int face = 0; face < 6; face++)
        {
            const int srcX = faceRegions[face].x * faceWidth;
            const int srcY = faceRegions[face].y * faceHeight;

            for (int row = 0; row < faceHeight; row++)
            {
                const auto srcOffset = (static_cast<size_t>(srcY + row) *
                    static_cast<size_t>(width) +
                    static_cast<size_t>(srcX)) *
                    static_cast<size_t>(channels);

                const auto destinationOffset = static_cast<size_t>(row) * faceRowBytes;

                std::memcpy(facePixels.data() + destinationOffset,
                    source + srcOffset, faceRowBytes);
            }

            glTextureSubImage3D(m_RenderID, 0, 0, 0, face, m_Size, m_Size,
                1, formaInfo.DataFormat, formaInfo.DataType, facePixels.data());
        }

        glTextureParameteri(m_RenderID, GL_TEXTURE_MIN_FILTER, 
            m_MipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

        glTextureParameteri(m_RenderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (options.GenerateMips)
            glGenerateTextureMipmap(m_RenderID);

        stbi_image_free(source);
    }
    OpenGLTextureCubeMap::~OpenGLTextureCubeMap()
    {
        if (m_RenderID != 0)
            glDeleteTextures(1, &m_RenderID);
    }
    void OpenGLTextureCubeMap::Bind(unsigned int slot) const
    {
        glBindTextureUnit(slot, m_RenderID);
    }
    OpenGLTexture2DArray::OpenGLTexture2DArray(unsigned int width, unsigned int height, unsigned int count, TextureFormat format)
		:m_Width(width), m_Height(height), m_Count(count), m_Format(format)
    {
        const auto info = GetOpenGLFormat(m_Format);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_RenderID);
		glTextureStorage3D(m_RenderID, 1, info.InternalFormat, width, height, m_Count);

        glTextureParameteri(m_RenderID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_RenderID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RenderID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }
    OpenGLTexture2DArray::~OpenGLTexture2DArray()
    {
        glDeleteTextures(1, &m_RenderID);
    }
    void OpenGLTexture2DArray::Bind(unsigned int slot) const
    {
        glBindTextureUnit(slot, m_RenderID);
    }
}
