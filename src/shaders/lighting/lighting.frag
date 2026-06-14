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

uniform vec3 uCamPos;
uniform mat4 uInvViewProj;

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

	// deferred pass: no per-vertex world position so rebuild this pixels eye
	// ray by sending it back thorugh inverse cam matrix.
	// keeps camera model entirely on the CPU
    vec3 ro = uCamPos;
    vec2 ndc = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
	vec4 farPoint = uInvViewProj * vec4(ndc, 1.0, 1.0);
	vec3 rd = normalize(farPoint.xyz / farPoint.w - ro);


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

	// The air is a shell wrapped around the planet
	// -> integrate scattering along the part of the eye ray inside the outer
	// atmosphere sphere, stopping at the terrain when the ray hits it
	vec2 shell = sphereIntersection(ro, rd, PLANET_RADIUS + ATMOSPHERE_HEIGHT);

    float sunCos = dot(rd, sun);
    float phaseR = PhaseRayleigh(sunCos);
    float phaseM = PhaseMie(sunCos, 0.6);

    vec2 opticalDistance = vec2(0.0);
    vec3 transmittance = vec3(1.0);
    vec3 scatteredLight = vec3(0.0);

	if (shell.y > 0.0) { // ray reaches the shell at all
		float marchStart = max(shell.x, 0.0); // clamp in case the camera is inside
		float marchEnd  = (t > 0.0) ? t : shell.y;
		float stepSize = (marchEnd - marchStart) / 16.0;

		for (float i = 0.0; i < 16.0; i++) {
			vec3 samplePos = ro + rd * (marchStart + (i + 0.5) * stepSize);	

			// altitude is distance out from the planet center, so density peaks
			// at the surface and fades to nothing at the top of the shell 
			float altitude = length(samplePos) - PLANET_RADIUS;
			float density = clamp01(1.0 - altitude / ATMOSPHERE_HEIGHT);

			float densityR = density * 1e5;
			float densityM = density * 1e5;
			
			opticalDistance += stepSize * vec2(densityR, densityM);
			transmittance = exp(-(opticalDistance.x * C_RAYLEIGH + opticalDistance.y * C_MIE));

			scatteredLight += transmittance * C_RAYLEIGH * phaseR * densityR * stepSize;
			scatteredLight += transmittance * C_MIE * phaseM * densityM * stepSize;
		}
    }

    color = color * transmittance + scatteredLight * 10.0;


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
