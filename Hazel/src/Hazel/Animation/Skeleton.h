#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Engine
{
	struct IKLink
	{
		uint32_t BoneIndex = 0;

		bool HasLimit = false;
		glm::vec3 MinAngle = glm::vec3(0.0f);
		glm::vec3 MaxAngle = glm::vec3(0.0f);
	};

	struct IKConstraint
	{
		uint32_t ControllerBoneIndex = UINT32_MAX;	// IK target
		uint32_t EffectorBoneIndex = UINT32_MAX;	// Bone pulled toward the IK target
			
		uint32_t Iterations = 0;
		float AngleLimit = 0.0f;					// Radians

		std::vector<IKLink> Links;					// Bones that the solver may rotate
	};

	// Model asset data. Read-only after initialization.
	struct BoneInfo		
	{
		std::string Name;
		int ParentIndex = -1;

		bool InheritRotation = false;
		bool InheritTranslation = false;
		int InheritParentIndex = -1;
		float InheritWeight = 0.0f;

		glm::mat4 OffsetMatrix = glm::mat4(1.0f);
		glm::mat4 LocalBindTransform = glm::mat4(1.0f);
		glm::mat4 GlobalBindTransform = glm::mat4(1.0f);
		std::vector<unsigned int> Children;
	};

	struct Skeleton
	{
		std::vector<BoneInfo> Bones;
		std::unordered_map<std::string, uint32_t> BoneNameToIndex;	// Fast lookup by bone name

		std::vector<IKConstraint> IKConstraints;
	};
}
