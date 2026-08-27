#pragma once

#include "Layer.h"

namespace Engine
{
	class HAZEL_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void Clear();

		void PushLayer(Scope<Layer> layer);
		void PushOverlay(Scope<Layer> overlay);
		void PopLayer(Scope<Layer> layer);
		void PopOverlayer(Scope<Layer> overlay);

		std::vector<Scope<Layer>>::iterator begin() { return m_Layers.begin(); }
		std::vector<Scope<Layer>>::iterator end() { return m_Layers.end(); }

		std::vector<Scope<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Scope<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }
	protected:
		std::vector<Scope<Layer>> m_Layers;
		int m_LayerInsertIndex;
	};
}

