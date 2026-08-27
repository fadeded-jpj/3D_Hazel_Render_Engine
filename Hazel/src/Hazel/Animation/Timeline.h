#pragma once

#include "Hazel/Core/Timestep.h"

#include <cmath>

namespace Engine
{
	class Timeline
	{
	public:
		inline void Play() { m_Playing = true; }
		inline void Pause() { m_Playing = false; }
		inline void Stop() { m_Playing = false; m_CurrentTime = 0.0f; }

		inline void SetTime(float time) { m_CurrentTime = time; }
		inline void SetDuration(float time) { m_Duration = time; }
		inline void SetPlaySpeed(float speed) { m_PlaybackSpeed = speed; }
		inline void SetLooping(bool loop) { m_Looping = loop; }

		inline void Update(Timestep ts)
		{
			if (!m_Playing)
				return;

			m_CurrentTime += ts.GetSeconds() * m_PlaybackSpeed;

			if (m_Duration > 0.0f && m_CurrentTime > m_Duration)
			{
				if (m_Looping)
					m_CurrentTime = std::fmod(m_CurrentTime, m_Duration);
				else
				{
					m_CurrentTime = m_Duration;
					m_Playing = false;
				}
			}
		}

		inline const float GetTime() const { return m_CurrentTime; }
		inline const float GetDuration() const { return m_Duration; }
		inline const bool IsPlaying() const { return m_Playing; }

	private:
		float m_CurrentTime = 0.0f;
		float m_Duration = 0.0f;
		float m_PlaybackSpeed = 1.0f;
		bool m_Playing = false;
		bool m_Looping = false;
	};
}