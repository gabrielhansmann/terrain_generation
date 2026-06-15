#version 460
// receives vWorldPos interpolated from water.vert (PLANET_HEIGHT)
#include "common.glsl"

uniform vec3 uCamPos;
in vec3 vWorldPos;

layout(location = 0) out vec4 gAlbedoOcclusion; // rgb=diffuseColor, a=occlusion
layout(location = 1) out vec4 gNormalMaterial; // rgb=normal, a=material/2.0
layout(location = 2) out vec4 gF0Smoothness; // r=f0, g=smoothness
layout(location = 3) out vec4 gDepth; // r=t, -1 for sky

void main() {
	vec3 normal = normalize(vWorldPos);

	// distance to eye
	float t = length(vWorldPos - uCamPos);

	// water is a dielectric: ~2% of light reflects head on (vs ~4% for rock
	// the rest refracts. high smoothness so the sun makes a tight glint
	// and scree-space reflections have something to read later
	vec3 f0 = vec3(0.02);
	float smoothness = 0.95;
	
    gAlbedoOcclusion = vec4(WATER_COLOR, 1.0);
    gNormalMaterial = vec4(normal, float(M_WATER) / 2.0);
    gF0Smoothness = vec4(f0.r, smoothness, 0.0, 0.0);
    gDepth = vec4(t, 0.0, 0.0, 0.0);
}
