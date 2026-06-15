#version 460 core
#include "common.glsl"

layout (location = 0) in vec3 aDir; 

uniform mat4 uViewProj;

out vec3 vWorldPos;

void main() {
	// water is just at planet radius
	vWorldPos = aDir * PLANET_RADIUS;
	gl_Position = uViewProj * vec4(vWorldPos, 1.0);
}
