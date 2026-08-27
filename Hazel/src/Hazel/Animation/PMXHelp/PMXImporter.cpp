#include "hzpch.h"
#include "PMXImporter.h"

#include <fstream>

#include "assimp/importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace Engine
{
	
	Ref<Skeleton> PMXSkeletonImporter::ImportFromFile(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath, std::ios::binary);

		if (!stream)
		{
			HZ_CORE_ERROR("Failed to open PMX file: {0}", filePath.string());
			return nullptr;
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
			return nullptr;
		}

		float version = 0.0f;
		readBytes(&version, sizeof(version));
		if (!valid || version < 2.0f || version >= 2.2f)
		{
			HZ_CORE_ERROR("Unsupported PMX version: {0}", version);
			return nullptr;
		}

		uint8_t headerSize = 0;
		readBytes(&headerSize, sizeof(headerSize));
		if (!valid || headerSize < 8)
		{
			HZ_CORE_ERROR("Invalid PMX header size: {0}", headerSize);
			return nullptr;
		}

		std::vector<uint8_t> headerData(headerSize);
		readBytes(headerData.data(), headerData.size());
		if (!valid)
		{
			HZ_CORE_ERROR("Failed to read PMX header: {0}", filePath.string());
			return nullptr;
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
			return nullptr;
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
		for (uint32_t i = 0; i < textureCount && valid; ++i)
			readText();

		const uint32_t materialCount = readCount("material");
		for (uint32_t i = 0; i < materialCount && valid; ++i)
		{
			readText();
			readText();

			// Diffuse, specular, specular strength, ambient, drawing flags,
			// edge color and edge size.
			skipBytes(sizeof(float) * 16ull + sizeof(uint8_t));
			skipBytes(textureIndexSize);
			skipBytes(textureIndexSize);
			skipBytes(sizeof(uint8_t)); // Sphere mode

			uint8_t sharedToon = 0;
			readBytes(&sharedToon, sizeof(sharedToon));
			if (sharedToon == 0)
				skipBytes(textureIndexSize);
			else if (sharedToon == 1)
				skipBytes(sizeof(uint8_t));
			else
				fail("Invalid PMX toon sharing flag");

			readText();
			skipBytes(sizeof(int32_t));
		}

		if (!valid)
		{
			HZ_CORE_ERROR("Failed to parse PMX file {0}: {1}", filePath.string(), errorMessage);
			return nullptr;
		}

		const uint32_t boneCount = readCount("bone");
		auto skeleton = std::make_shared<Skeleton>();
		skeleton->Bones.resize(boneCount);

		std::vector<glm::vec3> bonePositions(boneCount, glm::vec3(0.0f));

		constexpr uint16_t TailIsBone = 0x0001;
		constexpr uint16_t HasIK = 0x0020;
		constexpr uint16_t InheritRotation = 0x0100;
		constexpr uint16_t InheritTranslation = 0x0200;
		constexpr uint16_t FixedAxis = 0x0400;
		constexpr uint16_t LocalAxes = 0x0800;
		constexpr uint16_t ExternalParent = 0x2000;

		for (uint32_t boneIndex = 0; boneIndex < boneCount && valid; ++boneIndex)
		{
			std::string localName = readText();
			std::string universalName = readText();

			BoneInfo& bone = skeleton->Bones[boneIndex];
			bone.Name = !localName.empty()
				? std::move(localName)
				: (!universalName.empty()
					? std::move(universalName)
					: "PMXBone_" + std::to_string(boneIndex));

			float position[3]{};
			readBytes(position, sizeof(position));
			bonePositions[boneIndex] = {
				position[0],
				position[1],
				-position[2]
			};

			bone.ParentIndex = readSignedIndex(boneIndexSize);
			skipBytes(sizeof(int32_t)); // Deform layer

			uint16_t flags = 0;
			readBytes(&flags, sizeof(flags));

			if ((flags & TailIsBone) != 0)
				skipBytes(boneIndexSize);
			else
				skipBytes(sizeof(float) * 3ull);

			if ((flags & (InheritRotation | InheritTranslation)) != 0)
			{
				bone.InheritRotation = (flags & InheritRotation) != 0;
				bone.InheritTranslation = (flags & InheritTranslation) != 0;
				bone.InheritParentIndex = readSignedIndex(boneIndexSize);
				readBytes(&bone.InheritWeight, sizeof(bone.InheritWeight));

				if (bone.InheritParentIndex < 0 ||
					static_cast<uint32_t>(bone.InheritParentIndex) >= boneCount)
				{
					fail("PMX inherited transform parent index is out of range");
					break;
				}
			}

			if ((flags & FixedAxis) != 0)
				skipBytes(sizeof(float) * 3ull);

			if ((flags & LocalAxes) != 0)
				skipBytes(sizeof(float) * 6ull);

			if ((flags & ExternalParent) != 0)
				skipBytes(sizeof(int32_t));

			if ((flags & HasIK) != 0)
			{
				IKConstraint constraint;
				constraint.ControllerBoneIndex = boneIndex;

				const int32_t effectorIndex = readSignedIndex(boneIndexSize);
				if (effectorIndex < 0 || static_cast<uint32_t>(effectorIndex) >= boneCount)
				{
					fail("PMX IK effector index is out of range");
					break;
				}

				constraint.EffectorBoneIndex = static_cast<uint32_t>(effectorIndex);
				readBytes(&constraint.Iterations, sizeof(constraint.Iterations));
				readBytes(&constraint.AngleLimit, sizeof(constraint.AngleLimit));

				const uint32_t linkCount = readCount("IK link");
				constraint.Links.reserve(linkCount);

				for (uint32_t linkIndex = 0; linkIndex < linkCount && valid; ++linkIndex)
				{
					const int32_t linkedBoneIndex = readSignedIndex(boneIndexSize);
					if (linkedBoneIndex < 0 || static_cast<uint32_t>(linkedBoneIndex) >= boneCount)
					{
						fail("PMX IK link index is out of range");
						break;
					}

					IKLink link;
					link.BoneIndex = static_cast<uint32_t>(linkedBoneIndex);

					uint8_t hasLimit = 0;
					readBytes(&hasLimit, sizeof(hasLimit));
					link.HasLimit = hasLimit != 0;

					if (link.HasLimit)
					{
						float sourceMin[3]{};
						float sourceMax[3]{};
						readBytes(sourceMin, sizeof(sourceMin));
						readBytes(sourceMax, sizeof(sourceMax));

						const glm::vec3 convertedMin(
							-sourceMin[0],
							-sourceMin[1],
							sourceMin[2]);
						const glm::vec3 convertedMax(
							-sourceMax[0],
							-sourceMax[1],
							sourceMax[2]);

						link.MinAngle = glm::min(convertedMin, convertedMax);
						link.MaxAngle = glm::max(convertedMin, convertedMax);
					}

					constraint.Links.push_back(link);
				}

				skeleton->IKConstraints.push_back(std::move(constraint));
			}
		}

		if (!valid)
		{
			HZ_CORE_ERROR("Failed to parse PMX file {0}: {1}", filePath.string(), errorMessage);
			return nullptr;
		}

		for (uint32_t boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			BoneInfo& bone = skeleton->Bones[boneIndex];
			const glm::vec3 globalPosition = bonePositions[boneIndex];

			if (bone.ParentIndex >= 0 &&
				static_cast<uint32_t>(bone.ParentIndex) < boneCount &&
				static_cast<uint32_t>(bone.ParentIndex) != boneIndex)
			{
				const uint32_t parentIndex = static_cast<uint32_t>(bone.ParentIndex);
				skeleton->Bones[parentIndex].Children.push_back(boneIndex);
				const glm::vec3 localPosition = globalPosition - bonePositions[parentIndex];
				bone.LocalBindTransform = glm::mat4(1.0f);
				bone.LocalBindTransform[3] = glm::vec4(localPosition, 1.0f);
			}
			else
			{
				bone.ParentIndex = -1;
				bone.LocalBindTransform = glm::mat4(1.0f);
				bone.LocalBindTransform[3] = glm::vec4(globalPosition, 1.0f);
			}

			bone.GlobalBindTransform = glm::mat4(1.0f);
			bone.GlobalBindTransform[3] = glm::vec4(globalPosition, 1.0f);
			bone.OffsetMatrix = glm::mat4(1.0f);
			bone.OffsetMatrix[3] = glm::vec4(-globalPosition, 1.0f);

			auto result = skeleton->BoneNameToIndex.emplace(bone.Name, boneIndex);
			if (!result.second)
				HZ_CORE_WARN("Duplicate PMX bone name: {0}", bone.Name);
		}

		HZ_CORE_INFO(
			"PMX skeleton imported: bones={0}, IK constraints={1}",
			skeleton->Bones.size(),
			skeleton->IKConstraints.size());

		return skeleton;
	}

	
}
