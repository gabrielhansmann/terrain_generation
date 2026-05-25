#version 460 core
#include "common.glsl"

uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform vec3 iChannelResolution[4];

layout(location = 0) out vec4 gAlbedoOcclusion; // rgb=diffuseColor, a=occlusion
layout(location = 1) out vec4 gNormalMaterial; // rgb=normal, a=material/2.0
layout(location = 2) out vec4 gF0Smoothness; // r=f0, g=smoothness
layout(location = 3) out vec4 gDepth; // r=t, -1 for sky

#include "terrain.glsl"

// Get detail texture from Buffer B.
vec4 GetChannel1(vec2 uv) {
    uv *= BUFFER_SIZE / iChannelResolution[1].xy;
    return texture(iChannel1, uv);
}

// Get map channels from Buffer A, including the packed data.
vec3 GetChannel0Data(vec2 uv, out vec4 data) {
    vec2 p = uv * BUFFER_SIZE - 0.5;

    ivec2 i = ivec2(p);
    vec2 b = fract(p);
    vec2 a = 1.0 - b;

    // Get the four texels directly, rather than an interpolated result.
    vec4 AA = texelFetch(iChannel0, i, 0);
    vec4 AB = texelFetch(iChannel0, i + ivec2(0, 1), 0);
    vec4 BA = texelFetch(iChannel0, i + ivec2(1, 0), 0);
    vec4 BB = texelFetch(iChannel0, i + ivec2(1, 1), 0);
    // Use regular interpolation for the normal return value.
    // The w channel should not be used.
    vec4 ret = (AA * a.y + AB * b.y) * a.x + (BA * a.y + BB * b.y) * b.x;

    // Unpack each of the four samples into a vec4.
    vec4 AAdata = unpack4(AA.w);
    vec4 ABdata = unpack4(AB.w);
    vec4 BAdata = unpack4(BA.w);
    vec4 BBdata = unpack4(BB.w);
    // Interpolate the four vec4s so each piece of data is correctly interpolated.
    data = (AAdata * a.y + ABdata * b.y) * a.x + (BAdata * a.y + BBdata * b.y) * b.x;

    return ret.xyz;
}

// Get all map channels. This is needed for texturing and shading.
vec4 map(vec2 uv, out float erosion, out float ridgemap, out float trees, out float debug) {
    vec4 data;
    vec3 tex = GetChannel0Data(uv, data);

    float height = tex.x;

    // Calculate an accurate normal from the neighbouring points.
    vec2 uv1 = uv + vec2(1.0 / BUFFER_SIZE.x, 0.0);
    vec2 uv2 = uv + vec2(0.0, 1.0 / BUFFER_SIZE.y);
    float h1 = GetChannel0(uv1).x;
    float h2 = GetChannel0(uv2).x;
    vec3 v1 = vec3(uv1 - uv, (h1 - height));
    vec3 v2 = vec3(uv2 - uv, (h2 - height));
    vec3 normal = normalize(cross(v1, v2)).xzy;

    erosion  = data.x * 2.0 - 1.0;
    ridgemap = data.y;
    trees    = data.z;
    debug    = data.w;

    return vec4(height, normal);
}


void main() {
    // ------------------------------------------------------------------------
    // Set up camera
    // ------------------------------------------------------------------------

    vec3 ro;
    vec3 rd;
    GetRay(ro, rd, iTime, iMouse, iResolution, gl_FragCoord.xy, 0.0);


    // ------------------------------------------------------------------------
    // Ray march
    // ------------------------------------------------------------------------

    vec3 normal;
    int material;
    float s_t;
    float t = march(ro, rd, normal, material, s_t);

    if (t < 0.0) {
        gAlbedoOcclusion = vec4(0.0);
        gNormalMaterial  = vec4(0.0);
        gF0Smoothness    = vec4(0.0);
        gDepth           = vec4(-1.0, 0.0, 0.0, 0.0);
        return;
    }


    // ------------------------------------------------------------------------
    // Coloring and shading
    // ------------------------------------------------------------------------

    vec3 pos = ro + rd * t;

    float erosion;
    float ridgemap;
    float trees;
    float debug;
    vec4 mapData = map(GetUV(pos), erosion, ridgemap, trees, debug);
    float drainage = clamp01((1.0 - clamp01(ridgemap / DRAINAGE_WIDTH)) * 1.5);
    float diff = pos.y - mapData.x;

    float breakup = 0.0;
    #if DETAIL_TEXTURE
        vec4 breakupTex = GetChannel1(GetUV(pos));
        breakup = breakupTex.x;
        if (material == M_WATER) {
            normal.xz += breakupTex.zy * 0.1;
            normal = normalize(normal);
        }
    #endif

    vec3 f0 = vec3(0.04);
    float smoothness = 0.0;
    float occlusion = 1.0;

    // Color used when grayscale debug option is used:
    vec3 diffuseColor = vec3(0.5);

    if (material == M_GROUND) {
        normal = mapData.yzw;

        #if !GREYSCALE
            occlusion = clamp01(erosion + 0.5);

            // Cliffs / Dirt
            diffuseColor = CLIFF_COLOR * smoothstep(0.4, 0.52, pos.y);
            diffuseColor = mix(diffuseColor, DIRT_COLOR, smoothstep(0.6, 0.0, occlusion + breakup * 1.5));

            // Snow
            diffuseColor = mix(diffuseColor, vec3(1.0), smoothstep(0.53, 0.6, pos.y + breakup * 0.1));
            #if WATER
                // Sand (beach)
                diffuseColor = mix(diffuseColor, SAND_COLOR, smoothstep(WATER_HEIGHT + 0.005, WATER_HEIGHT, pos.y + breakup * 0.01));
            #endif

            // Grass
            vec3 grassMix = mix(GRASS_COLOR1, GRASS_COLOR2, smoothstep(0.4, 0.6, pos.y - erosion * 0.05 + breakup * 0.3));
            diffuseColor = mix(diffuseColor, grassMix,
                smoothstep(GRASS_HEIGHT + 0.05, GRASS_HEIGHT + 0.02, pos.y + 0.01 + (occlusion - 0.8) * 0.05 - breakup * 0.02)
                * smoothstep(0.8, 1.0, 1.0 - (1.0 - normal.y) * (1.0 - trees) + breakup * 0.1));

            // Trees
            float tree = max(0.0, trees * 2.0 - 1.0);
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
    }
    else if (material == M_STRATA) {
        #if !GREYSCALE
            vec3 strata = smoothstep(0.0, 1.0, cos(diff * vec3(130.0, 190.0, 250.0)));
            diffuseColor = vec3(0.3);
            diffuseColor = mix(diffuseColor, vec3(0.50), strata.x);
            diffuseColor = mix(diffuseColor, vec3(0.55), strata.y);
            diffuseColor = mix(diffuseColor, vec3(0.60), strata.z);

            diffuseColor *= exp(diff * 10.0) * vec3(1.0, 0.9, 0.7);
        #endif
    }
    else if (material == M_WATER) {
        float shore = normal.y > 1e-2 ? exp(-diff * 60.0) : 0.0;
        float foam  = normal.y > 1e-2 ? smoothstep(0.005, 0.0, diff + breakup * 0.005) : 0.0;

        diffuseColor = mix(WATER_COLOR, WATER_SHORE_COLOR, shore);

        diffuseColor = mix(diffuseColor, vec3(1.0), foam);

        //f0 = vec3(0.2);
        smoothness = 0.95;
    }

    gAlbedoOcclusion = vec4(diffuseColor, occlusion);
    gNormalMaterial = vec4(normal, float(material) / 2.0);
    gF0Smoothness = vec4(f0.r, smoothness, 0.0, 0.0);
    gDepth = vec4(t, 0.0, 0.0, 0.0);
}
