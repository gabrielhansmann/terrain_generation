#version 460 core
// renders one face of the terrain cubemap into a small panel
// this reads it the same way the sphere shader will eventually (by direction)
// so its ground truth for whether the compute step wrote the faces correctly
in vec2 vUV;
out vec4 fragColor;

uniform samplerCube uCube;
uniform int uFace;
uniform int uChannel; // -1 = rgb, 0..3 = show one channel as grey

#include "cube_face.glsl"

void main() {
	vec3 dir = cubeFaceDir(uFace, vUV);
	vec4 texel = texture(uCube, dir);
	vec3 color = (uChannel < 0) ? texel.rgb : vec3(texel[uChannel]);
	fragColor = vec4(color, 1.0);
}
