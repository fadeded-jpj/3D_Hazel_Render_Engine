#include "hzpch.h"
#include "AnimationClip.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Engine
{
	std::pair<glm::mat4, glm::mat4> AnimationClip::Interpolation(const BoneKeyFrame& frame1, const BoneKeyFrame& frame2, float time)
	{
		float alpha = (time - frame1.TimeSecond) / (frame2.TimeSecond - frame1.TimeSecond);

		if (alpha < 0.0f || alpha > 1.0f)
		{
			HZ_CORE_WARN("AnimationClip Interpolation Warning!");
			alpha = glm::clamp(alpha, 0.0f, 1.0f);
		}

		glm::vec3 trans = frame1.Translation * (1.0f - alpha) + frame2.Translation * alpha;
		glm::quat quat = glm::slerp(frame1.Rotation, frame2.Rotation, alpha);

		return { glm::translate(glm::mat4(1.0f), trans), glm::mat4_cast(glm::normalize(quat)) };
	}
	Ref<AnimationClip> AnimationClip::CreateTest()
	{
		Ref<AnimationClip> res = std::make_shared<AnimationClip>();
		res->Duration = 2.0f;

		const glm::quat identityRotation(1.0f, 0.0f, 0.0f, 0.0f);

		auto addTrack = [&](const std::string& boneName)
			{
				BoneAnimationTrack track;
				track.BoneName = boneName;

				BoneKeyFrame start;
				start.BondName = boneName;
				start.TimeSecond = 0.0f;
				start.Translation = glm::vec3(0.0f);
				start.Rotation = identityRotation;

				BoneKeyFrame up;
				up.BondName = boneName;
				up.TimeSecond = 1.0f;
				up.Translation = glm::vec3(0.0f, 10.0f, 0.0f);
				up.Rotation = identityRotation;

				BoneKeyFrame end;
				end.BondName = boneName;
				end.TimeSecond = 2.0f;
				end.Translation = glm::vec3(0.0f);
				end.Rotation = identityRotation;

				track.KeyFrame.push_back(start);
				track.KeyFrame.push_back(up);
				track.KeyFrame.push_back(end);

				res->BoneTracks[boneName] = track;
			};

		addTrack("root");
		addTrack("Root");
		addTrack("\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC");
		addTrack("\xE5\x85\xA8\xE3\x81\xA6\xE3\x81\xAE\xE8\xA6\xAA");

		return res;
	}
}
