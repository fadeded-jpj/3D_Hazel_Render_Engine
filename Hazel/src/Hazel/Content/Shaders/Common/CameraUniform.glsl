#define CAMERA

layout(std140, binding = 0) uniform CameraBlock
{
	mat4 View;
	mat4 Projection;
	mat4 InverseProjection;

	mat4 ViewProj;
	mat4 InverseViewProj;

	mat4 PreViewProj;

	mat4 JitteredViewProj;
	mat4 InverseJitteredViewProj;

	mat4 StabledViewProj;
	mat4 InverseStabledViewProj;

	vec4 CameraPositionAndTime;     // xyz: position, w: time
	vec4 ViewportSizeAndJittered;   // xy: viewport size, zw: current jitter
	vec4 CameraClip;                // xy: near/far, zw: previous jitter
} u_Camera;
