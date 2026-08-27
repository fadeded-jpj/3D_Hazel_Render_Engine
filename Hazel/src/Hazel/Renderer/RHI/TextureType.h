#pragma once

namespace Engine
{
	enum class TextureFormat
	{
		None = 0,

		R8,
		RG8,
		RGB8,
		RGBA8,

		RGB8_SRGB,
		RGBA8_SRGB,

		R16F,
		RG16F,
		RGB16F,
		RGBA16F,

		R32F,

		Depth24Stencil8,
		Depth32F
	};

	enum class TextureWrap
	{
		Repeat,
		ClampToEdge
	};


// --------------- color space enum ----------------
#define LIST(X) \
    X(Auto)             \
    X(Linear)           \
    X(SRGB)

	enum class TextureColorSpace
	{
#define X(name) name,
		LIST(X)
#undef X
	};

	inline std::string ToString(TextureColorSpace queue)
	{
		switch (queue)
		{
#define X(name) case TextureColorSpace::name: return #name;
			LIST(X)
#undef X

		default: return "Unknown";
		}
	}
	template<>
	inline std::optional<TextureColorSpace> FromString<TextureColorSpace>(std::string str)
	{

#define X(name) if(str == #name) return TextureColorSpace::name;
		LIST(X);
#undef X
		return std::nullopt;
	}

#undef LIST
}
