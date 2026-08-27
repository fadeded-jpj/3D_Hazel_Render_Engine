#include "hzpch.h"
#include "Scene.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderTypes.h"

namespace Engine
{
	namespace
	{
		AABB TransformAABB(const AABB& localBounds, const glm::mat4& transform)
		{
			const glm::vec3 localCenter =
				(localBounds.Min + localBounds.Max) * 0.5f;
			const glm::vec3 localExtent =
				(localBounds.Max - localBounds.Min) * 0.5f;

			const glm::vec3 worldCenter = glm::vec3(
				transform * glm::vec4(localCenter, 1.0f));
			const glm::mat3 linearTransform(transform);
			const glm::vec3 worldExtent =
				glm::abs(linearTransform[0]) * localExtent.x +
				glm::abs(linearTransform[1]) * localExtent.y +
				glm::abs(linearTransform[2]) * localExtent.z;

			return {
				worldCenter - worldExtent,
				worldCenter + worldExtent
			};
		}

		RenderDomain ResolveRenderDomain(EntityType type)
		{
			switch (type)
			{
			case EntityType::Actor:
				return RenderDomain::Character;

			case EntityType::Environment:
			case EntityType::Prop:
				return RenderDomain::Scene;

			case EntityType::Unknown:
			default:
				return RenderDomain::Scene;
			}
		}
	}


	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity(m_NextEntityID++, this);
		m_Entities.emplace(entity.GetID(),
			EntityRecord(name)
		);

		m_TransformComponents.Emplace(entity.GetID());
		return entity;
	}
	void Scene::DestroyEntity(Entity entity)
	{
		auto id = entity.GetID();
		m_TransformComponents.Remove(id);
		m_RenderComponents.Remove(id);
		m_AnimationComponents.Remove(id);
		m_LightComponents.Remove(id);
		m_Entities.erase(id);
	}
	bool Scene::IsEntityValid(Entity entity) const
	{
		return m_Entities.find(entity.GetID()) != m_Entities.end();
	}
	const std::string& Scene::GetEntityName(Entity entity) const
	{
		return m_Entities.at(entity.GetID()).name;
	}
	void Scene::SetEntityName(Entity entity, const std::string& name)
	{
		m_Entities.at(entity.GetID()).name = name;
	}
	EntityRecord& Scene::GetEntityRecord(Entity entity)
	{
		return m_Entities.at(entity.GetID());
	}
	EntityRecord& Scene::GetEntityRecord(EntityID id)
	{
		return m_Entities.at(id);
	}
	const EntityRecord& Scene::GetEntityRecord(Entity entity) const
	{
		return m_Entities.at(entity.GetID());
	}

	const EntityRecord& Scene::GetEntityRecord(EntityID id) const
	{
		return m_Entities.at(id);
	}

	void Scene::OnRender()
	{
		Renderer::SetSceneLighting(BuildSceneLightingData());

		for (auto& [entityID, _] : m_Entities)
		{
			const auto render = TryGetComponent<RenderComponent>(entityID);
			const auto type = GetEntityType(entityID);
			auto renderDomain = ResolveRenderDomain(type);

			if (!render || !render->ModelAsset)
				continue;

			const auto& model = render->ModelAsset;

			RenderSkinningData skinngData;
			const auto animator = TryGetComponent<AnimationComponent>(entityID);
			const auto transform = TryGetComponent<TransformComponent>(entityID);
			if (animator && animator->AnimatorAsset && animator->BoneBuffer)
			{
				//skinngData.HasSkeleton = 1;
				skinngData.BoneBuffer = animator->BoneBuffer;
				skinngData.PreviousBoneBuffer = animator->PreBoneBuffer;
			}
			const glm::mat4 entityTransform = transform->GetTransform();
			const AABB entityWorldBounds =
				TransformAABB(render->GetAABB(), entityTransform);

			if (renderDomain == RenderDomain::Character)
			{
				Renderer::SubmitCharacterBounds(entityWorldBounds);
			}

			// ------- for Face Shadow ------------
			RenderCharacterData character;
			glm::mat4 headModelTransform = glm::mat4(1.0f);
			character.HasHeadTransform =
				animator &&
				animator->AnimatorAsset &&
				animator->AnimatorAsset->TryGetBoneModelTransform(
					u8"\u982D", // "�^"
					headModelTransform);

			if (character.HasHeadTransform && type == EntityType::Actor)
			{
				const glm::mat4 headWorldTransform =
					entityTransform * headModelTransform;

				const glm::mat3 headRotation =
					glm::mat3(headWorldTransform);

				character.HeadPosition = headWorldTransform[3];

				const glm::vec3 localFaceForward(0.0f, 0.0f, 1.0f);
				const glm::vec3 localFaceRight(1.0f, 0.0f, 0.0f);
				const glm::vec3 localFaceUp(0.0f, 1.0f, 0.0f);

				character.HeadForward = glm::normalize(
					headRotation * localFaceForward);

				character.HeadRight = glm::normalize(
					headRotation * localFaceRight);

				character.HeadUp = glm::normalize(
					headRotation * localFaceUp);
			}
			// -----------------------------------

			auto RenderNode = [&](auto&& self, const Model::ModelNode& node, const glm::mat4& parentTransform)->void
				{
					glm::mat4 nodeTransform = parentTransform * node.LocalTransform;


					for (auto idx : node.MeshIndices)
					{
						const auto& subMesh = model->GetSubMeshes()[idx];
						auto material = render->GetMaterial(subMesh.MaterialIndex);

						SceneSubmitItem item;
						item.mesh = subMesh.Mesh;
						item.outlineEdgeMesh = subMesh.OutlineEdgeMesh;
						item.material = material;
						item.transform = entityTransform * nodeTransform;
						item.worldBounds = renderDomain == RenderDomain::Character
							? entityWorldBounds
							: TransformAABB(subMesh.LocalBounds, item.transform);
						item.skinningData = skinngData;
						item.CharacterData = character;
						item.renderObjectID = Renderer::MakeRenderObjectID(entityID, idx);
						item.outlinemode = render->OutlineMode;
						item.domain = renderDomain;

						Renderer::Submit(item);
					}

					for (const auto& child : node.Children)
						self(self, child, nodeTransform);
				};

			RenderNode(RenderNode, model->GetRootNode(), glm::mat4(1.0f));
		}
	}

	SceneLightingData Scene::BuildSceneLightingData() const
	{
		SceneLightingData res;

		res.Skybox = m_SkyBox;

		m_LightComponents.Each([&](EntityID id, const LightComponent& light)
			{
				if (!light.Enabled)
					return;
				if (res.LightCount >= SceneLightingData::MAX_LIGHTS)
					return;

				const auto* transform = m_TransformComponents.TryGet(id);

				HZ_CORE_ASSERT(transform, "Light requires TransformComponent");

				RenderLightResource& output = res.Lights[res.LightCount++];
				output.ColorIntensity = { light.Color, light.Intensity };
				output.PositionRange = { transform->GetTranslation(), light.Range };
				output.DirectionType = { transform->GetForward(), static_cast<float>(light.Type) };
				output.SpotAngles = { glm::cos(light.InnerConeAngle), glm::cos(light.OuterConeAngle), 0.0f, 0.0f };

			});

		return res;
	}



	void Scene::OnUpdate(Timestep ts)
	{
		m_AnimationSys.Update(ts);

		float time = m_AnimationSys.GetTime();
		for (auto& [id, data] : m_Entities)
		{
			const auto Animator = TryGetComponent<AnimationComponent>(id);
			if (Animator && Animator->AnimatorAsset && Animator->BoneBuffer)
			{
				Animator->AnimatorAsset->Update(m_AnimationSys.GetTime());
				const auto& matrices = Animator->AnimatorAsset->GetFinalBoneMatrices();
				if (!Animator->BoneHistoryValid)
				{
					Animator->BoneBuffer->SetData(matrices);
					Animator->PreBoneBuffer->SetData(matrices);
					Animator->BoneHistoryValid = true;
				}
				else
				{
					std::swap(Animator->BoneBuffer, Animator->PreBoneBuffer);
					Animator->BoneBuffer->SetData(matrices);
				}
			}
		}
	}

}
