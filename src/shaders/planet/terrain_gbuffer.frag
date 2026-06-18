#version 460
// receives vWorldPos interpolated from terrain.vert (a point on the displaced planet)
// samples the height cubemap by direction, build a world-space normal from
// height neighbours, writes the same 4 G-Buffer outputs as before.
// Strata branch dropped
// Water branch dropped for now
// -> They will get added as seperate geometry passes (water at least)
// Detail noise is dropped until it gets a spherical mapping.
// Coloring is the M_GROUND branch of the flat shader, re-anchored: "altitude"
// is the sampled height and "up" is the radial direction since pos.y is no longer
// altitude on a sphere
#include "common.glsl"
#include "plates.glsl"

uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;
uniform samplerCube iChannel0;

uniform vec3 uCamPos;

in vec3 vWorldPos;

layout(location = 0) out vec4 gAlbedoOcclusion; // rgb=diffuseColor, a=occlusion
layout(location = 1) out vec4 gNormalMaterial; // rgb=normal, a=material/2.0
layout(location = 2) out vec4 gF0Smoothness; // r=f0, g=smoothness
layout(location = 3) out vec4 gDepth; // r=t, -1 for sky

// #include "terrain.glsl"

void main() {
	// direction from planet center to this fragment (cubemap lookup key)
	vec3 dir = normalize(vWorldPos);

	// all four terrain values in one lookup, one per channel
	vec4 texel = texture(iChannel0, dir);
	float height = texel.x;
	float erosion = texel.y * 2.0 - 1.0;
	float ridgemap = texel.z;
	float trees = texel.w;

	// world-space normal from height neighbors -> these are small angular
	// steps around dir, not UV offsets. Two tangent directions span the local
	// surface, displace all three points by their height and cross the edges.
	// eps is about one heightmap texel
	vec3 up = abs(dir.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tan1 = normalize(cross(up, dir));
	vec3 tan2 = cross(dir, tan1);
	float eps = 1.0 / BUFFER_SIZE.x;

	vec3 dirA = normalize(dir + tan1 * eps);
	vec3 dirB = normalize(dir + tan2 * eps);
	float hA = texture(iChannel0, dirA).x;
	float hB = texture(iChannel0, dirB).x;

	vec3 pC = dir * (PLANET_RADIUS + height * HEIGHT_SCALE);
	vec3 pA = dirA * (PLANET_RADIUS + hA * HEIGHT_SCALE);
	vec3 pB = dirB * (PLANET_RADIUS + hB * HEIGHT_SCALE);
	vec3 normal = normalize(cross(pA - pC, pB - pC));
	if (dot(normal, dir) < 0.0) normal = -normal; //always point outward

	// distance from surface point to eye so lighting can rebuuld it along
	// the pixels ray. rasteriser already knows this point, dont rederive
	// a camera ray just to measure the distance
	float t = length(vWorldPos - uCamPos);

	// altitude replaces the flat shaders pos.y: slope is how far the surface tilts
	// fomr local up.
	float altitude = height;
	float slope = 1.0 - dot(normal, dir);
	
	float drainage = clamp01((1.0 - clamp01(ridgemap / DRAINAGE_WIDTH)) * 1.5);

	// detail break-up is dropped until it gets spherical mapping: kept a 0
	// so the coloring below is unchanged and detail returns cleanly later
	float breakup = 0.0;
	
	vec3 f0 = vec3(0.04);
    float smoothness = 0.0;
    float occlusion = 1.0;
    vec3 diffuseColor = vec3(0.5);

	// pos.y -> altitude (there is no pos variable in the new shader)
	// (1.0 - normal.y) -> slope
	#if !GREYSCALE
		occlusion = clamp01(erosion + 0.5);

		// Cliffs / Dirt
		diffuseColor = CLIFF_COLOR * smoothstep(0.4, 0.52, altitude);
		diffuseColor = mix(diffuseColor, DIRT_COLOR, smoothstep(0.6, 0.0, occlusion + breakup * 1.5));

		// Snow
		diffuseColor = mix(diffuseColor, vec3(1.0), smoothstep(0.53, 0.6, altitude + breakup * 0.1));
		#if WATER
			// Sand (beach)
			diffuseColor = mix(diffuseColor, SAND_COLOR, smoothstep(WATER_HEIGHT + 0.005, WATER_HEIGHT, altitude + breakup * 0.01));
		#endif

		// Grass
		vec3 grassMix = mix(GRASS_COLOR1, GRASS_COLOR2, smoothstep(0.4, 0.6, altitude - erosion * 0.05 + breakup * 0.3));
		diffuseColor = mix(diffuseColor, grassMix,
			smoothstep(GRASS_HEIGHT + 0.05, GRASS_HEIGHT + 0.02, altitude + 0.01 + (occlusion - 0.8) * 0.05 - breakup * 0.02)
			* smoothstep(0.8, 1.0, 1.0 - slope * (1.0 - trees) + breakup * 0.1));

		// Trees
		diffuseColor = mix(diffuseColor, TREE_COLOR * pow(trees, 8.0), clamp01(trees * 2.2 - 0.8) * 0.6);

		diffuseColor *= 1.0 + breakup * 0.5;
	#endif

	// Drainage (rivers, creeks, debris flow)
	#if DRAINAGE
		#if GREYSCALE
			diffuseColor = mix(diffuseColor, vec3(0.0, 2.5, 2.5), drainage);
		#else
			diffuseColor = mix(diffuseColor, vec3(1.0), drainage);
		#endif
	#endif
	// Mark pixels near cube-face edge so the per-face erosion seams show
	#if SHOW_SEAMS
		vec3 a = abs(dir);
		float largest = max(a.x, max(a.y, a.z));
		float smallest = min(a.x, min(a.y, a.z));
		float middle = a.x + a.y + a.z - largest - smallest;
		float distToEdge = 1.0 - middle / largest; // 0 at the seam
		float seamBand = 6.0 * (2.0 / BUFFER_SIZE.x); // ~6 texels wide
		diffuseColor = mix(vec3(1.0, 0.0, 1.0), diffuseColor, smoothstep(0.0, seamBand, distToEdge));
    #endif
	// paint the plate partition with a distinct hue per plate
	#if SHOW_PLATES
		vec2 plate = sphericalVoronoi(dir, PLATE_COUNT);
		vec3 plateColor = 0.5 + 0.5 * cos(plate.x * 2.4 + vec3(0.0, 2.1, 4.2));
		diffuseColor = plateColor;
	#endif
		

    // Color used when grayscale debug option is used:
    gAlbedoOcclusion = vec4(diffuseColor, occlusion);
	// was: vec4(normal, float(material) / 2.0);
	// now: M_GROUND instead of material since water got dropped
    gNormalMaterial = vec4(normal, float(M_GROUND) / 2.0);
    gF0Smoothness = vec4(f0.r, smoothness, 0.0, 0.0);
    gDepth = vec4(t, 0.0, 0.0, 0.0);
}
