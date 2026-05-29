#version 460 core
#include "common.glsl"

layout (location = 0) in vec2 aPos;

uniform sampler2D iChannel0;
uniform vec3 iChannelResolution[4];
uniform vec3 iResolution;
uniform mat4 uViewProj;

out vec3 vWorldPos;

// naming:
// a = attribute (vertex input)
// u = uniform (CPU-supplied constant)
// v = varying (output interpolated to fragment shader)

#include "terrain.glsl"

void main() {
	vec2 uv = GetUV(vec3(aPos.x, 0.0, aPos.y));
	uv *= BUFFER_SIZE / iChannelResolution[0].xy;
	float height = textureLod(iChannel0, uv, 0).x;

	vWorldPos = vec3(aPos.x, height, aPos.y);
	gl_Position = uViewProj * vec4(vWorldPos, 1.0);
}
