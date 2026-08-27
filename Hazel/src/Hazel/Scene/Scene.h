#pragma once

#include "Hazel/Camera/CameraController.h"
#include "Entity.h"
#include "Components.h"
#include "Hazel/Animation/AnimationSystem.h"
#include "Hazel/Scene/ComponentStorge.h"
#include "Hazel/Renderer/Lighting/LightType.h"

namespace Engine
{
	struct EntityRecord
	{
		std::string name;
		EntityType type;
		EntityTag tag;

		EntityRecord(const std::string& name, EntityType type = EntityType::Unknown,
			EntityTag tag = EntityTag::None)
			:name(name), type(type), tag(tag)
		{}
	};

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		Entity CreateEntity(const std::string& name);
		void DestroyEntity(Entity entity);
		
		bool IsEntityValid(Entity entity) const;
		
		const std::string& GetEntityName(Entity entity) const;
		void SetEntityName(Entity entity, const std::string& name);

		EntityRecord& GetEntityRecord(Entity entity);
		EntityRecord& GetEntityRecord(EntityID id);
		const EntityRecord& GetEntityRecord(EntityID id) const;
		const EntityRecord& GetEntityRecord(Entity entity) const;

		template<typename T>
		ComponentStorage<T>& GetStorage()
		{
			if constexpr (std::is_same_v<T, TransformComponent>)
				return m_TransformComponents;
			else if constexpr (std::is_same_v<T, RenderComponent>)
				return m_RenderComponents;
			else if constexpr (std::is_same_v<T, AnimationComponent>)
				return m_AnimationComponents;
			else if constexpr (std::is_same_v<T, LightComponent>)
				return m_LightComponents;
			else
				static_assert(false, "Component type is not registered in Scene");
		}

		template<typename T, typename... Args>
		T& AddComponent(EntityID id, Args&&... args)
		{
			T& component = GetStorage<T>().Emplace(id, std::forward<Args>(args)...);

			if constexpr (std::is_same_v<T, AnimationComponent>)
			{
				if (auto* render = m_RenderComponents.TryGet(id))
					render->BoundsScale = RenderComponent::AnimatedBoundsScale;
			}
			else if constexpr (std::is_same_v<T, RenderComponent>)
			{
				if (m_AnimationComponents.Contains(id))
					component.BoundsScale = RenderComponent::AnimatedBoundsScale;
			}

			return component;
		}

		template<typename T>
		T* TryGetComponent(EntityID id)
		{
			return GetStorage<T>().TryGet(id);
		}

		template<typename T>
		const T* TryGetComponent(EntityID id) const
		{
			return GetStorage<T>().TryGet(id);
		}


		inline std::unordered_map<EntityID, EntityRecord>& GetEntities() { return m_Entities; }
		
		inline AnimationSystem& GetAnimationSystem() { return m_AnimationSys; }

		inline EntityType GetEntityType(const EntityID& id) 
		{ 
			auto it = m_Entities.find(id);
			if (it == m_Entities.end())
			{
				HZ_CORE_ERROR("Entity {0} not exised !", id);
				return EntityType::Unknown;
			}
			return it->second.type; 
		}
		inline bool SetEntityType(const EntityID& id, EntityType type)
		{
			auto it = m_Entities.find(id);
			if (it == m_Entities.end())
				return false;

			it->second.type = type;
			return true;
		}

		inline EntityTag GetEntityTag(const EntityID& id)
		{
			auto it = m_Entities.find(id);
			if (it == m_Entities.end())
			{
				HZ_CORE_ERROR("Entity {0} not exised !", id);
				return EntityTag::None;
			}
			return it->second.tag;
		}
		inline bool SetEntityTag(const EntityID& id, EntityTag tag)
		{
			auto it = m_Entities.find(id);
			if (it == m_Entities.end())
				return false;

			it->second.tag = tag;
			return true;
		}

		inline void SetSkyBox(const Ref<TextureCubeMap>& skybox)
		{
			m_SkyBox = skybox;
		}
		
		void OnUpdate(Timestep ts);
		void OnRender();
		void OnEvent(Event& e);
	private: 
		SceneLightingData BuildSceneLightingData() const;


	private:
		EntityID m_NextEntityID = 1;
		std::unordered_map<EntityID, EntityRecord> m_Entities;
		std::unordered_map<EntityID, EntityType> m_EntityTypes;

		ComponentStorage<TransformComponent> m_TransformComponents;
		ComponentStorage<RenderComponent> m_RenderComponents;
		ComponentStorage<AnimationComponent> m_AnimationComponents;
		ComponentStorage<LightComponent> m_LightComponents;
		
		AnimationSystem m_AnimationSys;

		Ref<TextureCubeMap> m_SkyBox;
	};

}


