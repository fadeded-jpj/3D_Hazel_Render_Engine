#include "hzpch.h"
#include "VMDImport.h"

#include <fstream>

namespace Engine
{
#pragma pack(push, 1)
	struct VMDBoneFrameRaw
	{
		char BoneName[15];
		uint32_t FrameIndex;
		float Position[3];
		float Rotation[4];

		uint8_t Interpolation[64];
	};
#pragma pack(pop)
	static_assert(sizeof(VMDBoneFrameRaw) == 111);

	std::string VMDImporter::DecodeShiftJIS(const char* data, uint32_t count)
	{
		size_t len = 0;
		while (len < count && data[len] != '\0')
			len++;
		
		if (len == 0)
			return "";

		int widthLen = MultiByteToWideChar(932, 0, data, (int)len, nullptr, 0);

		std::wstring wide(widthLen, L'\0');
		MultiByteToWideChar(932, 0, data, (int)len, wide.data(), widthLen);

		int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), widthLen, nullptr, 0, nullptr, nullptr);
		std::string utf8(utf8Len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide.data(), widthLen, utf8.data(), utf8Len, nullptr, nullptr);

		return utf8;
	}

	Ref<AnimationClip> VMDImporter::ImportFromFile(const std::string& filepath)
	{
		std::ifstream in(filepath, std::ios::binary);

		if (!in)
			return nullptr;
		
		char header[30];
		char modelName[20];

		in.read(header, sizeof(header));
		in.read(modelName, sizeof(modelName));

		unsigned int boneFrameCount = 0;
		in.read(reinterpret_cast<char*>(&boneFrameCount), sizeof(unsigned int));

		auto clip = std::make_shared<AnimationClip>();
		for (unsigned int i = 0; i < boneFrameCount; i++)
		{
			VMDBoneFrameRaw raw;
			in.read(reinterpret_cast<char*>(&raw), sizeof(raw));

			std::string boneName = DecodeShiftJIS(raw.BoneName, 15);

			BoneKeyFrame key;
			key.BondName = boneName;
			key.TimeSecond = raw.FrameIndex / 30.0f;
			key.Translation = glm::vec3(
				raw.Position[0], 
				raw.Position[1], 
				-raw.Position[2]);
			key.Rotation = glm::normalize(glm::quat(
				raw.Rotation[3], 
				-raw.Rotation[0], 
				-raw.Rotation[1], 
				raw.Rotation[2]));

			auto& track = clip->BoneTracks[boneName];
			track.BoneName = boneName;
			track.KeyFrame.push_back(key);

			clip->Duration = std::max(clip->Duration, key.TimeSecond);
		}

		for (auto& [_, track] : clip->BoneTracks)
		{
			std::sort(track.KeyFrame.begin(), track.KeyFrame.end(), [](const BoneKeyFrame& a, const BoneKeyFrame& b)
				{
					return a.TimeSecond < b.TimeSecond;
				});
		}

		HZ_CORE_INFO("VMD bone frame count: {0}", boneFrameCount);
		HZ_CORE_INFO("VMD bone track count: {0}", clip->BoneTracks.size());
		HZ_CORE_INFO("VMD duration: {0}", clip->Duration);

		return clip;
	}
}