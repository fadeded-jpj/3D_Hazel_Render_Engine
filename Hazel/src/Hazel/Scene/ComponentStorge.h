#pragma once
#include <unordered_map>
#include <utility>

#include "Hazel/Core/Core.h"
#include "Entity.h"

namespace Engine
{
	template<typename T>
	class ComponentStorage
	{
	public:
		template<typename... Args>
		T& Emplace(EntityID id, Args&&... args)
		{
			auto [it, inserted] = m_Components.try_emplace(id, std::forward<Args>(args)...);

			HZ_CORE_ASSERT(inserted, "Entity already has this component");
			return it->second;
		}

		bool Contains(EntityID id) const
		{
			return m_Components.find(id) != m_Components.end();
		}

		T& Get(EntityID id)
		{
			auto it = m_Components.find(id);
			HZ_CORE_ASSERT(it != m_Components.end(), "Entity does not have this component");
			return it->second;
		}

		T* TryGet(EntityID id)
		{
			auto it = m_Components.find(id);
			return it != m_Components.end() ? &it->second : nullptr;
		}

		const T* TryGet(EntityID id) const
		{
			auto it = m_Components.find(id);
			return it != m_Components.end() ? &it->second : nullptr;
		}
	
		bool Remove(EntityID id)
		{
			return m_Components.erase(id) > 0;
		}

		template<typename Func>
		void Each(Func&& func)
		{
			for (auto& [id, v] : m_Components)
				func(id, v);
		}

		template<typename Func>
		void Each(Func&& func) const
		{
			for (const auto& [entityID, component] : m_Components)
				func(entityID, component);
		}

	private:
		std::unordered_map<EntityID, T> m_Components;
	};
}