#version 460 core
#include "common.glsl"

uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;
uniform sampler2D iChannel0; // heightmap — needed for shadow and reflection marches
uniform sampler2D iChannel2; // dither noise
uniform vec3 iChannelResolution[4];

uniform sampler2D gAlbedoOcclusion;
uniform sampler2D gNormalMaterial;
uniform sampler2D gF0Smoothness;
uniform sampler2D gDepth;

out vec4 FragColor;

#include "terrain.glsl"

void main() {
    // ------------------------------------------------------------------------
    // Read G-buffer
    // ------------------------------------------------------------------------

    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 albedoOcc = texelFetch(gAlbedoOcclusion, coord, 0);
    vec4 normalMat = texelFetch(gNormalMaterial, coord, 0);
    vec4 f0Smooth = texelFetch(gF0Smoothness, coord, 0);
    float t = texelFetch(gDepth, coord, 0).r;

    vec3 diffuseColor = albedoOcc.rgb;
    float occlusion = albedoOcc.a;
    vec3 normal = normalMat.rgb;
    int material = int(round(normalMat.a * 2.0));
    vec3 f0 = vec3(f0Smooth.r);
    float smoothness = f0Smooth.g;


    // ------------------------------------------------------------------------
    // Set up camera
    // ------------------------------------------------------------------------

    vec3 ro;
    vec3 rd;
    GetRay(ro, rd, iTime, iMouse, iResolution, gl_FragCoord.xy, 0.0);


    // ------------------------------------------------------------------------
    // Coloring and shading
    // ------------------------------------------------------------------------

    float iRevolution = iTime * 2.0 * PI;
    vec2 cameraAngle = vec2(
        iRevolution * TIME_CAM_SPIN
            + sin(iRevolution / 6.0) * TIME_CAM_WOBBLE * 6.0
            + CAMERA_ANGLE * 2.0 * PI,
        CAMERA_ELEVATION);

    #if CAMERA_MOUSE_CONTROL
        if (iMouse.z > 0.5) {
            vec2 mouse = iMouse.xy / iResolution.xy;
            mouse.y = clamp01(mix(mouse.y, 0.5, -1.0));
            cameraAngle = (mouse - vec2(0.5, 1.0)) * vec2(-PI * 2.0, PI * 0.5);
        }
    #endif

    mat3 rot = CameraRotation(cameraAngle.yx);

    #if FIXED_SUN
        vec3 sun = normalize(vec3(-1.0, 0.4, 0.05));
    #else
        vec3 sun = rot * normalize(vec3(-1.0, 0.15, 0.25));
    #endif

    vec3 fogColor = 1.0 - exp(-SkyColor(rd, sun) * 2.0);

    vec3 color;

    if (t < 0.0) {
        // Sky
        color = fogColor * (1.0 + pow(gl_FragCoord.y / iResolution.y, 3.0) * 3.0) * 0.5;
        #if SHOW_NORMALS
            color = vec3(0.5, 0.5, 1.0);
        #endif
    }
    else {
        vec3 pos = ro + rd * t;

        vec3 r = reflect(rd, normal);

        float shadow = 1.0;

        #if SHADOWS
            if (material != M_STRATA) {
                // Shadow ray
                vec3 foo;
                float s_t;
                int s_material;
                march(pos + vec3(0.0, 1.0, 0.0) * 1e-4, sun, foo, s_material, s_t);
                shadow = 1.0 - exp(-s_t * 20.0);
            }
        #endif

        // Ambient
        color = diffuseColor * SkyColor(normal, sun) * Fd_Lambert();
        color *= occlusion;
        // Direct
        color += Shade(diffuseColor, f0, smoothness, normal, -rd, sun, SUN_COLOR * shadow);
        // Bounce
        color += diffuseColor * SUN_COLOR
            * (dot(normal, sun * vec3(1.0, -1.0, 1.0)) * 0.5 + 0.5)
            * Fd_Lambert() / PI;
        // Reflection
		// Only shiny surfaces reflect and since water is pushed to later, skip
		// this for now (also keeps flat-box march from running one terrain
		// becomes a sphere)
		if (smoothness > 0.0) {
			color += GetReflection(pos, r, sun, smoothness)
				* F_Schlick(f0, dot(-rd, normal));
		}

        #if SHOW_DIFFUSE
            color = pow(diffuseColor, vec3(1.0 / 2.2));
        #elif SHOW_NORMALS
            color = normal.xzy * 0.5 + 0.5;
        #endif
    }


    // ------------------------------------------------------------------------
    // Atmosphere rendering
    // ------------------------------------------------------------------------

    vec3 boxNormal;
    vec2 box = boxIntersection(ro, rd, boxSize, boxNormal);

    float costh = dot(rd, sun);
    float phaseR = PhaseRayleigh(costh);
    float phaseM = PhaseMie(costh, 0.6);

    vec2 od = vec2(0.0);
    vec3 tsm = vec3(1.0);
    vec3 sct = vec3(0.0);
    float rayLength = (t > 0.0 ? t : box.y) - box.x;
    float stepSize = rayLength / 16.0;
    for (float i = 0.0; i < 16.0; i++) {
        vec3 p = ro + rd * (box.x + (i + 0.5) * stepSize);

        float h = max(0.0, p.y - 0.35);
        float d = 1.0 - clamp01(h / 0.2);

        if (p.y < 0.35) {
            d = 0.0;
        }

        float densityR = d * 1e5;
        float densityM = d * 1e5;

        od += stepSize * vec2(densityR, densityM);

        tsm = exp(-(od.x * C_RAYLEIGH + od.y * C_MIE));

        sct += tsm * C_RAYLEIGH * phaseR * densityR * stepSize;
        sct += tsm * C_MIE * phaseM * densityM * stepSize;
    }

    color = color * tsm + sct * 10.0;


    // ------------------------------------------------------------------------
    // Tone mapping and dithering
    // ------------------------------------------------------------------------

    #if !SHOW_NORMALS && !SHOW_DIFFUSE
        color = Tonemap_ACES(color);
        color = pow(color, vec3(1.0 / 2.2));
    #endif

    // Dither
    vec2 channel2Res   = iChannelResolution[2].xy;
    vec2 channel2Tiled = mod(gl_FragCoord.xy, channel2Res) / channel2Res;
    color += texture(iChannel2, channel2Tiled).xxx / 255.0;

    FragColor = vec4(color, 1.0);
}
