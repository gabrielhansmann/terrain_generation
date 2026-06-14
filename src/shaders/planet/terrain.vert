#version 460 core
#include "common.glsl"

layout (location = 0) in vec3 aDir; // unit direction of this cube-sphere vertex

uniform samplerCube iChannel0; // heihght cubemap; .x = terrain height for a direction
uniform mat4 uViewProj;

out vec3 vWorldPos;

// naming:
// a = attribute (vertex input)
// u = uniform (CPU-supplied constant)
// v = varying (output interpolated to fragment shader)

void main() {
	// height is looked up by the direction this vertex points, not by a flat XZ
	// position. 
	float height = textureLod(iChannel0, aDir, 0.0).x;

	// lift the vertex out from the planet center along its own direction: a base
	// sphere of PLANET_RADIUS (common.glsl) within a thin height shell on top.
	// HEIGHT_SCALE (common.glsl) stays small so relief reads as terrain
	vWorldPos = aDir * (PLANET_RADIUS + height * HEIGHT_SCALE);
	gl_Position = uViewProj * vec4(vWorldPos, 1.0);
}
