#include "hzpch.h"
#include "ShadowMap.h"

#include "Hazel/Renderer/Material/MaterialInstance.h"

namespace Engine
{
	ShadowMap::ShadowMap(unsigned int width, unsigned int height)
	{
		m_ShadowMap = FrameBuffer::Create({
			width, height,
			{{TextureFormat::Depth24Stencil8}}
			});
	}
	PointShadowMap::PointShadowMap(unsigned int size)
	{
		m_DepthCubeMap = TextureCubeMap::Create(size, TextureFormat::Depth32F);
		m_ShadowMap = FrameBuffer::Create({
			size, size, {}, true
			});
	}
	CSMShadowMap::CSMShadowMap(unsigned int width, unsigned int height, unsigned int cascadeCount)
	{
		m_ShadowMap = FrameBuffer::Create(
			{
				width, height,
				{}, true
			}
		);
		m_DepthArray = Texture2DArray::Create(width, height, cascadeCount, 
			TextureFormat::Depth32F);
	}
}