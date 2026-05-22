// shared heightmap sampling and marching helpers
// include after common.glsl. Calling shader must decalare:
// 		uniform sampler2D iChannel0;
// 		uniform vec3 iChannelResolution[4];


// functions below copied from terrain_flat.frag

vec3 boxSize = vec3(0.5, 1.0, 0.5);

vec3 GetChannel0(vec2 uv) {
	uv *= BUFFER_SIZE / iChannelResolution[0].xy;
	return texture(iChannel0, uv).xyz;
}

vec2 GetUV(vec3 p) {
	vec2 pixel = vec2(1.0) / BUFFER_SIZE;
	vec2 uv = p.xz * (1.0 - pixel * 2.0) + vec2(0.5);
	uv = clamp(uv, pixel, vec2(1.0) - pixel);
	return uv;
}

float mapHeight(vec2 uv) {
	return GetChannel0(uv).x;
}

float march(vec3 ro, vec3 rd, out vec3 normal, out int material, out float s_t) {
    s_t = 9999.0;
    
    vec3 boxNormal;
    vec2 box = boxIntersection(ro, rd, boxSize, boxNormal);
    
    float tStart = max(0.0, box.x) + 1e-2;
    float tEnd = box.y - 1e-2;
    
    material = M_GROUND;

    float stepSize = 0.0;
    float stepScale = 1.0 / RAYMARCH_QUALITY;
    int samples = int(48.0 * RAYMARCH_QUALITY);
    float t = tStart;
    float altitude = 0.0;
    for (int i = 0; i < samples; i++) {
        vec3 pos = ro + rd * t;
        
        float h = mapHeight(GetUV(pos));
        altitude = pos.y - h;
        
        s_t = max(0.0, min(s_t, altitude / t));
        
        if (altitude < 0.0) {
            if (i < 1) { // Sides
                if (pos.y < 0.35) { // Flat bottom
                    s_t = 9999.0;
                    return -1.0;
                }
                normal = boxNormal;
                material = M_STRATA;
                break;
            }
        }
        
        if (altitude < 0.0) {
            // Step back (contact/edge refinement).
            stepScale *= 0.5;
            t -= stepSize * stepScale;
        }
        else {
            // Step forward
            // Accelerate the ray by distance to terrain.
            // This would result in horrible aliasing if we didn't do refinement above.
            //stepSize = abs(altitude) + 1e-2;
            stepSize = abs(altitude) + min(1e-2, abs(altitude) * 0.01);
            //stepSize = (tEnd - tStart) / float(stepCount);
            t += stepSize * stepScale;
        }
    }
    
    #ifdef WATER
        vec3 waterNormal;
        vec2 water = boxIntersection(ro, rd, vec3(boxSize.x, WATER_HEIGHT, boxSize.z), waterNormal);
        if ((water.y > 0.0 && (water.x < t || t < 0.0)) && material != M_STRATA) {
            t = max(0.0, water.x);
            normal = waterNormal;
            material = M_WATER;
        }
    #endif    

    if (box.y < 0.0) {
        s_t = 9999.0;
        return -1.0;
    }
    
    if (t > tEnd) {
        return -1.0;
    }

    return t;
}

vec3 GetReflection(vec3 p, vec3 r, vec3 sun, float smoothness) {
    vec3 refl = SkyColor(r, sun) * 4.0;
    vec3 foo;
    float r_t;
    int r_material;
    march(p, r, foo, r_material, r_t);
    return refl * (1.0 - exp(-r_t * 10.0 * sq(smoothness)));
}
