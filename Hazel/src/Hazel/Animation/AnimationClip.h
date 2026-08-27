#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Hazel/AssetsSystem/AssetType.h"

namespace Engine
{
	struct BoneKeyFrame
	{
		std::string BondName;

		float TimeSecond = 0.0f;
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	};

	struct BoneAnimationTrack
	{
		std::string BoneName;
		std::vector<BoneKeyFrame> KeyFrame;
	};


	class AnimationClip : public Asset
	{
	public:
		float Duration = 0.0f;
		std::unordered_map<std::string, BoneAnimationTrack> BoneTracks;

		static std::pair<glm::mat4, glm::mat4> Interpolation(const BoneKeyFrame& frame1, const BoneKeyFrame& frame2, float time);
		static Ref<AnimationClip> CreateTest();

		ASSET_TYPE(AnimationClip);
	};
}