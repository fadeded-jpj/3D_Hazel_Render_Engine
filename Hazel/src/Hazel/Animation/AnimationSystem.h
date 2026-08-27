#pragma once

#include "Timeline.h"

namespace Engine
{
	class AnimationSystem
	{
	public:
		inline void Play() { m_Timeline.Play(); }
		inline void Pause() { m_Timeline.Pause(); }
		inline void Stop() { m_Timeline.Stop(); }

		inline const float GetTime() const { return m_Timeline.GetTime(); }
		inline const float GetDuration() const { return m_Timeline.GetDuration(); }
		inline const bool IsPlaying() const { return m_Timeline.IsPlaying(); }

		inline void SetTime(float time) { m_Timeline.SetTime(time); }
		inline void SetDuration(float time) { m_Timeline.SetDuration(time); }
		inline void SetLoop(bool loop) { m_Timeline.SetLooping(loop); }
		inline void SetPlayingSpeed(float speed) { m_Timeline.SetPlaySpeed(speed); }

		inline void Update(Timestep ts) { m_Timeline.Update(ts); }

	private:
		Timeline m_Timeline;
	};
}