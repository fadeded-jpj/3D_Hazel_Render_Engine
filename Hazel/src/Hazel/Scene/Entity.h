#pragma once

namespace Engine
{
	class Scene;

	using EntityID = uint32_t;
	constexpr EntityID INVALID_ENTITY_ID = 0;

	enum class EntityType
	{
		Unknown = 0, Actor, Environment, Prop, Camera
	};

	enum class EntityTag : uint32_t
	{
		None = 0,
		MainCharacter = BIT(0),
		Background = BIT(1),
		EditorOnly = BIT(2),
		Static = BIT(3)
	};

	class Entity
	{
	public:
		Entity() = default;
		Entity(EntityID id, Scene* scene)
			:m_ID(id), m_Scene(scene) {
		}

		inline EntityID GetID() const { return m_ID; }
		inline Scene* GetScene() const { return m_Scene; }

		inline bool operator==(const Entity& other) const { return m_ID == other.m_ID && m_Scene == other.m_Scene; }
		inline bool operator!=(const Entity& other) const { return !(*this == other); }
		//template<typename T>
		//T& GetComponent()
		//{
		//	//return m_Scene->GetEntityData(*this).GetComponent<T>();
		//	
		//}

	private:
		EntityID m_ID = INVALID_ENTITY_ID;
		Scene* m_Scene = nullptr;
	};
}