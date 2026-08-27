#include "hzpch.h"
#include "PMXMaterialReader.h"

#include <fstream>


namespace Engine
{
	PMXMaterialImportData PMXMaterialReader::Read(const std::filesystem::path& filePath)
	{
		PMXMaterialImportData result;
		std::ifstream stream(filePath, std::ios::binary);

		if (!stream)
		{
			HZ_CORE_ERROR("Failed to open PMX file: {0}", filePath.string());
			return {};
		}

		bool valid = true;
		std::string errorMessage;

		auto fail = [&](const std::string& message)
			{
				if (valid)
					errorMessage = message;
				valid = false;
			};

		auto readBytes = [&](void* destination, size_t size)
			{
				if (!valid)
					return;

				stream.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(size));
				if (!stream)
					fail("Unexpected end of file");
			};

		auto skipBytes = [&](uint64_t size)
			{
				if (!valid)
					return;

				stream.seekg(static_cast<std::streamoff>(size), std::ios::cur);
				if (!stream)
					fail("Unexpected end of file while skipping a PMX section");
			};

		auto readSignedIndex = [&](uint8_t indexSize)
			{
				if (indexSize == 1)
				{
					int8_t value = -1;
					readBytes(&value, sizeof(value));
					return static_cast<int32_t>(value);
				}

				if (indexSize == 2)
				{
					int16_t value = -1;
					readBytes(&value, sizeof(value));
					return static_cast<int32_t>(value);
				}

				if (indexSize == 4)
				{
					int32_t value = -1;
					readBytes(&value, sizeof(value));
					return value;
				}

				fail("Invalid PMX index size");
				return int32_t(-1);
			};

		auto readCount = [&](const char* sectionName)
			{
				int32_t count = 0;
				readBytes(&count, sizeof(count));

				if (count < 0 || count > 100000000)
				{
					fail(std::string("Invalid PMX ") + sectionName + " count");
					return uint32_t(0);
				}

				return static_cast<uint32_t>(count);
			};

		uint8_t textEncoding = 0;
		auto readText = [&]()
			{
				int32_t byteLength = 0;
				readBytes(&byteLength, sizeof(byteLength));

				if (byteLength < 0 || byteLength > 64 * 1024 * 1024)
				{
					fail("Invalid PMX text length");
					return std::string();
				}

				if (byteLength == 0)
					return std::string();

				if (textEncoding == 1)
				{
					std::string text(static_cast<size_t>(byteLength), '\0');
					readBytes(text.data(), text.size());
					return text;
				}

				if (textEncoding != 0 || (byteLength % 2) != 0)
				{
					fail("Invalid PMX text encoding");
					return std::string();
				}

				std::wstring wideText(static_cast<size_t>(byteLength / 2), L'\0');
				readBytes(wideText.data(), static_cast<size_t>(byteLength));

				if (!valid || wideText.empty())
					return std::string();

				const int utf8Length = WideCharToMultiByte(
					CP_UTF8,
					0,
					wideText.data(),
					static_cast<int>(wideText.size()),
					nullptr,
					0,
					nullptr,
					nullptr);

				if (utf8Length <= 0)
				{
					fail("Failed to convert PMX UTF-16 text to UTF-8");
					return std::string();
				}

				std::string result(static_cast<size_t>(utf8Length), '\0');
				WideCharToMultiByte(
					CP_UTF8,
					0,
					wideText.data(),
					static_cast<int>(wideText.size()),
					result.data(),
					utf8Length,
					nullptr,
					nullptr);
				return result;
			};

		char signature[4]{};
		readBytes(signature, sizeof(signature));
		if (!valid ||
			signature[0] != 'P' ||
			signature[1] != 'M' ||
			signature[2] != 'X' ||
			signature[3] != ' ')
		{
			HZ_CORE_ERROR("Invalid PMX signature: {0}", filePath.string());
			return {};
		}

		float version = 0.0f;
		readBytes(&version, sizeof(version));
		if (!valid || version < 2.0f || version >= 2.2f)
		{
			HZ_CORE_ERROR("Unsupported PMX version: {0}", version);
			return {};
		}

		uint8_t headerSize = 0;
		readBytes(&headerSize, sizeof(headerSize));
		if (!valid || headerSize < 8)
		{
			HZ_CORE_ERROR("Invalid PMX header size: {0}", headerSize);
			return {};
		}

		std::vector<uint8_t> headerData(headerSize);
		readBytes(headerData.data(), headerData.size());
		if (!valid)
		{
			HZ_CORE_ERROR("Failed to read PMX header: {0}", filePath.string());
			return {};
		}

		textEncoding = headerData[0];
		const uint8_t additionalUVCount = headerData[1];
		const uint8_t vertexIndexSize = headerData[2];
		const uint8_t textureIndexSize = headerData[3];
		const uint8_t materialIndexSize = headerData[4];
		const uint8_t boneIndexSize = headerData[5];
		const uint8_t morphIndexSize = headerData[6];
		const uint8_t rigidBodyIndexSize = headerData[7];

		auto isValidIndexSize = [](uint8_t size)
			{
				return size == 1 || size == 2 || size == 4;
			};

		if (textEncoding > 1 ||
			additionalUVCount > 4 ||
			!isValidIndexSize(vertexIndexSize) ||
			!isValidIndexSize(textureIndexSize) ||
			!isValidIndexSize(materialIndexSize) ||
			!isValidIndexSize(boneIndexSize) ||
			!isValidIndexSize(morphIndexSize) ||
			!isValidIndexSize(rigidBodyIndexSize))
		{
			HZ_CORE_ERROR("Invalid PMX global header settings: {0}", filePath.string());
			return {};
		}

		// Local name, universal name, local comment and universal comment.
		for (uint32_t i = 0; i < 4; ++i)
			readText();

		const uint32_t vertexCount = readCount("vertex");
		for (uint32_t i = 0; i < vertexCount && valid; ++i)
		{
			skipBytes((3 + 3 + 2 + additionalUVCount * 4ull) * sizeof(float));

			uint8_t deformType = 0;
			readBytes(&deformType, sizeof(deformType));

			switch (deformType)
			{
			case 0: // BDEF1
				skipBytes(boneIndexSize);
				break;
			case 1: // BDEF2
				skipBytes(boneIndexSize * 2ull + sizeof(float));
				break;
			case 2: // BDEF4
			case 4: // QDEF
				skipBytes(boneIndexSize * 4ull + sizeof(float) * 4ull);
				break;
			case 3: // SDEF
				skipBytes(boneIndexSize * 2ull + sizeof(float) * 10ull);
				break;
			default:
				fail("Unsupported PMX vertex deform type");
				break;
			}

			skipBytes(sizeof(float)); // Edge scale
		}

		const uint32_t surfaceIndexCount = readCount("surface index");
		skipBytes(static_cast<uint64_t>(surfaceIndexCount) * vertexIndexSize);

		const uint32_t textureCount = readCount("texture");
		result.Textures.reserve(textureCount);
		for (uint32_t i = 0; i < textureCount && valid; ++i)
			result.Textures.emplace_back(std::filesystem::u8path(readText()));

		const uint32_t materialCount = readCount("material");
		result.Materials.reserve(materialCount);
		for (uint32_t i = 0; i < materialCount && valid; ++i)
		{
			PMXMaterialSupplement material;
			material.name = readText();
			readText();

			// Assimp already imports diffuse, specular and ambient values.
			skipBytes(sizeof(float) * 11ull);

			uint8_t drawingFlags = 0;
			readBytes(&drawingFlags, sizeof(drawingFlags));
			material.DrawingFlags = drawingFlags;

			float edgeColor[4]{};
			readBytes(edgeColor, sizeof(edgeColor));
			material.EdgeColor = {
				edgeColor[0], edgeColor[1], edgeColor[2], edgeColor[3]
			};
			readBytes(&material.EdgeSize, sizeof(material.EdgeSize));

			material.BaseTextureIndex = readSignedIndex(textureIndexSize);
			material.SphereTextureIndex = readSignedIndex(textureIndexSize);
			readBytes(&material.SphereMode, sizeof(material.SphereMode));

			uint8_t sharedToon = 0;
			readBytes(&sharedToon, sizeof(sharedToon));
			if (sharedToon == 0)
			{
				material.Toon.Shared = false;
				material.Toon.TextureIndex = readSignedIndex(textureIndexSize);
			}
			else if (sharedToon == 1)
			{
				uint8_t sharedIndex = 0;
				readBytes(&sharedIndex, sizeof(sharedIndex));
				material.Toon.Shared = true;
				material.Toon.SharedIndex = sharedIndex;
			}
			else
				fail("Invalid PMX toon sharing flag");

			readText();
			skipBytes(sizeof(int32_t));
			result.Materials.push_back(std::move(material));
		}

		if (!valid)
		{
			HZ_CORE_ERROR("Failed to parse PMX file {0}: {1}", filePath.string(), errorMessage);
			return {};
		}

		return result;
	}
}
