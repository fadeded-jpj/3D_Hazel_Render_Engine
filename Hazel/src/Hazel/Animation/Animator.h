#pragma once

#include "Hazel/Renderer/Model/Model.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Animation/AnimationClip.h"

namespace Engine
{
	struct BonePose
	{
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		glm::mat4 LocalTransform = glm::mat4(1.0f);
		glm::mat4 GlobalTransform = glm::mat4(1.0f);
	};

	class Animator
	{
	public:
		Animator(const Ref<Model>& model);

		void Update(float time);
		const std::vector<glm::mat4>& GetFinalBoneMatrices() const;

		void SetAnimationClip(Ref<AnimationClip> clip);
		bool TryGetBoneModelTransform(const std::string& boneName, glm::mat4& transform) const;

	private:
		glm::mat4 CacLocalTransform(const BoneInfo& Bone, float time);
		std::pair<glm::vec3, glm::quat> GetLocalTranslateAndRotation(const BoneInfo& Bone, float time);
		void UpdateFKBoneTransform(unsigned int index, const glm::mat4& parentAnimatedGlobal, float time);
		std::pair<glm::mat4, glm::mat4> SampleTrackMat(const BoneAnimationTrack& track, float time); // return {translate, rotation}
		std::pair<glm::vec3, glm::quat> SampleTrack(const BoneAnimationTrack& track, float time);
		
		// for update
		void SampleFKAnimation(float time);
		void SolveCCDIK(unsigned int maxStep = 100);
		void ApplyInheritedTransforms();
		void BuildFinalMatrices();

		// for IK solver
		void RebuildLocalTransform(uint32_t index);
		void UpdateGlobalTransformRecursive(uint32_t index);
		void SolveSingleLink(const IKConstraint& ik, const IKLink& link);

		bool IsIKTargetReached(const IKConstraint& ik);

	private:
		Ref<Model> m_Model;	// Read-only
		std::vector<glm::mat4> m_FinalBoneMatrices;

		std::vector<BonePose> m_BonePoses;

		Ref<AnimationClip> m_AnimationClip;
	};
}
