#include "hzpch.h"
#include "Animator.h"
#include "VMDHelp/VMDImport.h"

namespace Engine
{
	static std::string ToHexString(const std::string& text)
	{
		std::string result;

		char buffer[8];
		for (unsigned char c : text)
		{
			sprintf_s(buffer, "\\x%02X", c);
			result += buffer;
		}

		return result;
	}
	void Animator::SetAnimationClip(Ref<AnimationClip> clip) 
	{ 
		m_AnimationClip = clip; 

		int matched = 0;
		for (auto& [name, track] : clip->BoneTracks)
		{
			if (m_Model->GetSkeleton()->BoneNameToIndex.find(name) != m_Model->GetSkeleton()->BoneNameToIndex.end())
				matched++;
		}
		HZ_CORE_INFO("VMD matched bones: {0}/{1}", matched, clip->BoneTracks.size());
	}

	bool Animator::TryGetBoneModelTransform(const std::string& boneName, glm::mat4& transform) const
	{
		transform = glm::mat4(1.0f);
		if (!m_Model)
			return false;
		const auto& skeleton = m_Model->GetSkeleton();
		if (!skeleton)
			return false;

		const auto& it = skeleton->BoneNameToIndex.find(boneName);
		if (it == skeleton->BoneNameToIndex.end())
		{
			HZ_CORE_ERROR("{0} Bone not find!", boneName.c_str());
			return false;
		}
		auto boneIndex = it->second;
		transform = m_Model->GetGlobalInverseTransform()* m_BonePoses[boneIndex].GlobalTransform;
		return true;
	}

	Animator::Animator(const Ref<Model>& model)
		:m_Model(model)
	{
		int boneCount = 0;
		if (m_Model && m_Model->GetSkeleton())
		{
			boneCount = (int)m_Model->GetSkeleton()->Bones.size();
		}

		m_FinalBoneMatrices.resize(boneCount, glm::mat4(1.0f));
		m_BonePoses.resize(boneCount);
	}
	const std::vector<glm::mat4>& Animator::GetFinalBoneMatrices() const
	{
		return m_FinalBoneMatrices;
	}

	glm::mat4 Animator::CacLocalTransform(const BoneInfo& bone, float time)
	{
		glm::mat4 localTransform = bone.LocalBindTransform;
		if (m_AnimationClip)
		{
			// Temporary name aliases for models without PMX inherited transforms.
			const BoneAnimationTrack* track = nullptr;
			
			auto trackIt = m_AnimationClip->BoneTracks.find(bone.Name);
			if (trackIt != m_AnimationClip->BoneTracks.end())
				track = &trackIt->second;
			else if (!bone.InheritRotation && !bone.InheritTranslation)
			{
				auto mapIt = VMDBoneNameMap.find(bone.Name);
				if (mapIt != VMDBoneNameMap.end())
				{
					auto fallbackIt = m_AnimationClip->BoneTracks.find(mapIt->second);
					if (fallbackIt != m_AnimationClip->BoneTracks.end())
						track = &fallbackIt->second;
				}
			}

			if (track)
			{
				auto [T, R] = SampleTrackMat(*track, time);
				localTransform = bone.LocalBindTransform * T * R;
			}
		}

		return localTransform;
	}

	std::pair<glm::vec3, glm::quat> Animator::GetLocalTranslateAndRotation(const BoneInfo& bone, float time)
	{
		glm::vec3 translate = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		if (m_AnimationClip)
		{
			// Temporary name aliases for models without PMX inherited transforms.
			const BoneAnimationTrack* track = nullptr;

			auto trackIt = m_AnimationClip->BoneTracks.find(bone.Name);
			if (trackIt != m_AnimationClip->BoneTracks.end())
				track = &trackIt->second;
			else if (!bone.InheritRotation && !bone.InheritTranslation)
			{
				auto mapIt = VMDBoneNameMap.find(bone.Name);
				if (mapIt != VMDBoneNameMap.end())
				{
					auto fallbackIt = m_AnimationClip->BoneTracks.find(mapIt->second);
					if (fallbackIt != m_AnimationClip->BoneTracks.end())
						track = &fallbackIt->second;
				}
			}

			if (track)
			{
				auto [T, R] = SampleTrack(*track, time);
				translate = T;
				rotation = R;
			}
		}
		return { translate, rotation };
	}

	void Animator::UpdateFKBoneTransform(unsigned int index, const glm::mat4& parentAnimatedGlobal, float time)
	{
		const auto& skeleton = m_Model->GetSkeleton();
		const auto& bones = skeleton->Bones;
		const auto& bone = bones[index];
		auto& pose = m_BonePoses[index];

		auto [T, R] = GetLocalTranslateAndRotation(bone, time);

		pose.Translation = T;
		pose.Rotation = R;
		pose.LocalTransform = bone.LocalBindTransform *
			glm::translate(glm::mat4(1.f), T) * glm::mat4_cast(glm::normalize(R));
		pose.GlobalTransform = parentAnimatedGlobal * pose.LocalTransform;


		for (auto& child : bone.Children)
			UpdateFKBoneTransform(child, pose.GlobalTransform, time);
	}

	std::pair<glm::mat4, glm::mat4> Animator::SampleTrackMat(const BoneAnimationTrack& track, float time)
	{
		auto& frames = track.KeyFrame;

		if (frames.empty())
			return { glm::mat4(1.0f) , glm::mat4(1.0f) };

		if (frames.size() == 1 || time <= frames.front().TimeSecond)
			return { glm::translate(glm::mat4(1.0f), frames[0].Translation), glm::mat4_cast(glm::normalize(frames[0].Rotation)) };

		if(time >= frames.back().TimeSecond)
			return { glm::translate(glm::mat4(1.0f), frames.back().Translation), glm::mat4_cast(glm::normalize(frames.back().Rotation)) };

		auto upper = std::upper_bound(
			frames.begin(),
			frames.end(),
			time,
			[](float t, const BoneKeyFrame& frame)
			{
				return t < frame.TimeSecond;
			}
		);

		const auto& k0 = *(upper - 1);
		const auto& k1 = *upper;

		return AnimationClip::Interpolation(k0, k1, time);
	}

	std::pair<glm::vec3, glm::quat> Animator::SampleTrack(const BoneAnimationTrack& track, float time)
	{
		auto& frames = track.KeyFrame;

		if (frames.empty())
			return { glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};

		if (frames.size() == 1 || time <= frames.front().TimeSecond)
			return { frames[0].Translation, glm::normalize(frames[0].Rotation) };

		if (time >= frames.back().TimeSecond)
			return { frames.back().Translation, glm::normalize(frames.back().Rotation) };

		auto upper = std::upper_bound(
			frames.begin(),
			frames.end(),
			time,
			[](float t, const BoneKeyFrame& frame)
			{
				return t < frame.TimeSecond;
			}
		);

		const auto& k0 = *(upper - 1);
		const auto& k1 = *upper;

		auto fn = [](const BoneKeyFrame& frame1, const BoneKeyFrame& frame2, float time)->std::pair<glm::vec3, glm::quat>
			{

				float alpha = (time - frame1.TimeSecond) / (frame2.TimeSecond - frame1.TimeSecond);

				if (alpha < 0.0f || alpha > 1.0f)
				{
					HZ_CORE_WARN("AnimationClip Interpolation Warning!");
					alpha = glm::clamp(alpha, 0.0f, 1.0f);
				}

				glm::vec3 trans = frame1.Translation * (1.0f - alpha) + frame2.Translation * alpha;
				glm::quat quat = glm::normalize(glm::slerp(frame1.Rotation, frame2.Rotation, alpha));
				return { trans, quat };
			};

		return fn(k0, k1, time);
	}

	void Animator::SampleFKAnimation(float time)
	{
		const auto& skeleton = m_Model->GetSkeleton();
		const auto& bones = skeleton->Bones;


		for (auto i = 0; i < bones.size(); i++)
		{
			if (bones[i].ParentIndex == -1)
				UpdateFKBoneTransform(i, glm::mat4(1.0f), time);
		}
	}

	void Animator::SolveCCDIK(unsigned int maxStep)
	{
		const auto& skeleton = m_Model->GetSkeleton();

		for (const auto& ik : skeleton->IKConstraints)
		{
			const uint32_t iterationCount = glm::min(maxStep, ik.Iterations);
			for (uint32_t step = 0; step < iterationCount; ++step)
			{
				for (const auto& link : ik.Links)
				{
					SolveSingleLink(ik, link);
					if (IsIKTargetReached(ik))
						break;
				}

				if (IsIKTargetReached(ik))
					break;
			}
		}
	}

	void Animator::BuildFinalMatrices()
	{
		const auto& skeleton = m_Model->GetSkeleton();
		const auto& bones = skeleton->Bones;


		for (auto i = 0; i < bones.size(); i++)
		{
			m_FinalBoneMatrices[i] = m_Model->GetGlobalInverseTransform() *
				m_BonePoses[i].GlobalTransform *
				bones[i].OffsetMatrix;
		}
	}

	void Animator::ApplyInheritedTransforms()
	{
		const auto& bones = m_Model->GetSkeleton()->Bones;
		const glm::quat identityRotation(1.0f, 0.0f, 0.0f, 0.0f);

		for (uint32_t index = 0; index < bones.size(); ++index)
		{
			const BoneInfo& bone = bones[index];
			if ((!bone.InheritRotation && !bone.InheritTranslation) ||
				bone.InheritParentIndex < 0 ||
				static_cast<uint32_t>(bone.InheritParentIndex) >= m_BonePoses.size())
			{
				continue;
			}

			BonePose& pose = m_BonePoses[index];
			const BonePose& sourcePose = m_BonePoses[bone.InheritParentIndex];

			if (bone.InheritRotation)
			{
				const glm::quat inheritedRotation = glm::slerp(
					identityRotation,
					glm::normalize(sourcePose.Rotation),
					bone.InheritWeight);

				pose.Rotation = glm::normalize(pose.Rotation * inheritedRotation);
			}

			if (bone.InheritTranslation)
				pose.Translation += sourcePose.Translation * bone.InheritWeight;

			RebuildLocalTransform(index);
			UpdateGlobalTransformRecursive(index);
		}
	}

	void Animator::RebuildLocalTransform(uint32_t index)
	{
		const auto& bone = m_Model->GetSkeleton()->Bones[index];
		auto& pose = m_BonePoses[index];

		pose.LocalTransform = bone.LocalBindTransform
			* glm::translate(glm::mat4(1.0f), pose.Translation) 
			* glm::mat4_cast(pose.Rotation);
	}

	void Animator::UpdateGlobalTransformRecursive(uint32_t index)
	{
		const auto& bone = m_Model->GetSkeleton()->Bones[index];
		auto& pose = m_BonePoses[index];

		if (bone.ParentIndex >= 0)
		{
			pose.GlobalTransform = m_BonePoses[bone.ParentIndex].GlobalTransform * pose.LocalTransform;
		}
		else
			pose.GlobalTransform = pose.LocalTransform;

		for (auto child : bone.Children)
			UpdateGlobalTransformRecursive(child);
	}

	void Animator::SolveSingleLink(const IKConstraint& ik, const IKLink& link)
	{
		const auto linkIndex = link.BoneIndex;
		const auto targetIndex = ik.ControllerBoneIndex;
		const auto effectorIndex = ik.EffectorBoneIndex;

		if (linkIndex >= m_BonePoses.size() || targetIndex >= m_BonePoses.size() || effectorIndex >= m_BonePoses.size())
		{
			HZ_CORE_ERROR("IK Link dismatch!");
			return;
		}
		
		auto& linkPose = m_BonePoses[linkIndex];

		const glm::vec3 targetPosition = glm::vec3(m_BonePoses[targetIndex].GlobalTransform[3]);
		const glm::vec3 effectorPosition = glm::vec3(m_BonePoses[effectorIndex].GlobalTransform[3]);

		constexpr float epsilon = 1e-8f;

		// Convert positions into the current link's local space.
		const glm::mat4 inverseLinkGlobal = glm::inverse(linkPose.GlobalTransform);

		glm::vec3 effectorDirector = glm::vec3(inverseLinkGlobal * glm::vec4(effectorPosition, 1.0f));
		glm::vec3 targetDirector = glm::vec3(inverseLinkGlobal * glm::vec4(targetPosition, 1.0f));

		const float effectorLength2 = glm::dot(effectorDirector, effectorDirector);
		const float targetLength2 = glm::dot(targetDirector, targetDirector);

		if (effectorLength2 < epsilon || targetLength2 < epsilon)
		{
			HZ_CORE_ERROR("IK caculate error for bone {0}!", linkIndex);
			return;
		}

		effectorDirector = glm::normalize(effectorDirector);
		targetDirector = glm::normalize(targetDirector);

		// Compute the angular correction between both directions.
		const float cosine = glm::clamp(glm::dot(effectorDirector, targetDirector), -1.0f, 1.0f);
		float angle = glm::acos(cosine);

		if (angle < epsilon)
			return;
		
		if (ik.AngleLimit > 0.0f)
			angle = glm::min(angle, ik.AngleLimit);

		glm::vec3 axis = glm::cross(effectorDirector, targetDirector);
		const float axisLength2 = glm::dot(axis, axis);

		// Parallel directions need a stable fallback axis.
		if (axisLength2 < epsilon)
		{
			if (cosine > 0.0f)
			{
				return;
			}

			axis = glm::cross(effectorDirector, glm::vec3(1.0f, 0.0f, 0.0f));
			if (glm::dot(axis, axis) < epsilon)
			{
				axis = glm::cross(effectorDirector, glm::vec3(0.0f, 1.0f, 0.0f));
			}
		}

		axis = glm::normalize(axis);

		const glm::quat deltaRotation = glm::angleAxis(angle, axis);
		linkPose.Rotation = glm::normalize(linkPose.Rotation * deltaRotation);

		if (link.HasLimit)
		{
			glm::vec3 euler = glm::eulerAngles(linkPose.Rotation);

			euler.x = glm::clamp(euler.x, link.MinAngle.x, link.MaxAngle.x);
			euler.y = glm::clamp(euler.y, link.MinAngle.y, link.MaxAngle.y);
			euler.z = glm::clamp(euler.z, link.MinAngle.z, link.MaxAngle.z);
		
			linkPose.Rotation = glm::normalize(glm::quat(euler));
		}

		RebuildLocalTransform(linkIndex);
		UpdateGlobalTransformRecursive(linkIndex);
	}

	bool Animator::IsIKTargetReached(const IKConstraint& ik)
	{
		if (ik.ControllerBoneIndex >= m_BonePoses.size() || ik.EffectorBoneIndex >= m_BonePoses.size())
			return false;

		const glm::vec3 targetPosition = m_BonePoses[ik.ControllerBoneIndex].GlobalTransform[3];
		const glm::vec3 effectorPositon = m_BonePoses[ik.EffectorBoneIndex].GlobalTransform[3];

		float distance = glm::dot(targetPosition - effectorPositon, targetPosition - effectorPositon);
		return distance < 0.0001;
	}


	void Animator::Update(float time)
	{
		SampleFKAnimation(time);
		SolveCCDIK();
		ApplyInheritedTransforms();	// 将动画应用于 D 骨
		BuildFinalMatrices();
	}
}
