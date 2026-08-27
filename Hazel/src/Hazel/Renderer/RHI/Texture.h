#pragma once

#include <string>
#include <filesystem>
#include "Hazel/Core/Core.h"

#include "Hazel/AssetsSystem/AssetType.h"
#include "TextureType.h"
#include <glm/glm.hpp>

namespace Engine
{
	// GPU level
	

	struct TextureLoadOptions
	{
		TextureColorSpace ColorSpace = TextureColorSpace::Linear;
		bool GenerateMips = true;
		bool FlipVertically = false;
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		glm::uvec2 GetMipSize(uint32_t mipLevel) const
		{
			HZ_CORE_ASSERT(
				mipLevel < GetMipLevelCount(),
				"Texture mip level is out of range");

			return {
				std::max(1u, GetWidth() >> mipLevel),
				std::max(1u, GetHeight() >> mipLevel)
			};
		}

		static uint32_t CalculateMipLevelCount(
			uint32_t width,
			uint32_t height)
		{
			return 1u + static_cast<uint32_t>(
				std::floor(std::log2(std::max(width, height))));
		}

		virtual void Bind(unsigned int slot = 0) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		// create from file
		static Ref<Texture2D> Create(const std::filesystem::path& path, 
			const TextureLoadOptions& options = {});

		// create empty texture
		static Ref<Texture2D> Create(int width, int height, TextureFormat format,
			uint32_t mipLevels = 1, TextureWrap wrap = TextureWrap::ClampToEdge);

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void Bind(unsigned int slot = 0) const = 0;

		virtual Ref<Texture2D> Clone() = 0;
		virtual void CopyFrom(const Ref<Texture2D>& source) = 0;

		ASSET_TYPE(Texture2D);
	};

	class TextureCubeMap : public Texture
	{
	public:
		// create from file
		static Ref<TextureCubeMap> Create(const std::filesystem::path& path, const TextureLoadOptions& options = {});

		// create empty texture
		static Ref<TextureCubeMap> Create(unsigned int size, TextureFormat format);

		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetWidth() const { return GetSize(); }
		virtual uint32_t GetHeight() const { return GetSize(); }
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual void Bind(unsigned int slot = 0) const = 0;

		ASSET_TYPE(TextureCubeMap);
	};

	class Texture2DArray : public Texture
	{
	public:
		// static Ref<TextureCubeMap> Create(const std::filesystem::path& path, const TextureLoadOptions& options = {});

		// create empty texture
		static Ref<Texture2DArray> Create(unsigned int width, unsigned int height
			, unsigned int count, TextureFormat format);

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;
		virtual void Bind(unsigned int slot = 0) const = 0;

	private:
		ASSET_TYPE(Texture2DArray);
	};
}
