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


    vec3 color;

    if (t < 0.0) {
        // planet sits in space -> background is black
		// angle between eye ray and sun in radians
		float sunAngle = acos(clamp(dot(rd, sun), -1.0, 1.0));
		float sunDisc = 1.0 - smoothstep(SUN_DISC_SIZE * (1.0 - SUN_DISC_SOFTNESS), SUN_DISC_SIZE, sunAngle);
        color = SUN_COLOR * SUN_INTENSITY * sunDisc;
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

        // Sky light is sunlight scattered through the air, so it only reaches
		// the day side and the twilight band near the terminator. The night 
		// face turns away from the lit atmosphere and goes dark. Without this
		// factor, the ambient dome lights the whole globe evenly and there is 
		// no night.
		float skyLight = smoothstep(-0.3, 0.2, dot(normal, sun));
        color = diffuseColor * SkyColor(normal, sun) * Fd_Lambert() * skyLight;
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
		// if (smoothness > 0.0) {
		// 	color += GetReflection(pos, r, sun, smoothness)
		// 		* F_Schlick(f0, dot(-rd, normal));
		// }

        #if SHOW_DIFFUSE
            color = pow(diffuseColor, vec3(1.0 / 2.2));
        #elif SHOW_NORMALS
            color = normal.xzy * 0.5 + 0.5;
        #endif
    }


    // ------------------------------------------------------------------------
    // Atmosphere rendering
    // ------------------------------------------------------------------------

	// single-scattering model ported from Dimas Leenman's
	// "atmospheric-scattering-explained" (atmosphere.glsl, MIT license).
	// Its adapted to our units (plaet radius = 1, not metres) and our 
	// sphereIntersection helper. atmosphere.glsl:NN references point at that file
	vec2 shell = sphereIntersection(ro, rd, PLANET_RADIUS + ATMOSPHERE_HEIGHT);

    float sunCos = dot(rd, sun);
    float phaseR = PhaseRayleigh(sunCos);
	// The Mie sun-glow must not shine thorugh the ssolid planet, so switch off
	// (atmosphere.glsl:140,170)
    float phaseM = (t < 0.0) ? PhaseMie(sunCos, 0.76) : 0.0;

	// rayleigh and mie air corssed by the eye ray so far, kept in seperate channels
	// since each color is extinguished by a different amount
	// -> this split is what reddens light at the terminator
    vec3 opticalDistance = vec3(0.0);
    vec3 transmittance = vec3(1.0);
    vec3 scatteredRayleigh = vec3(0.0);
	vec3 scatteredMie = vec3(0.0);

	if (shell.y > 0.0) { // ray reaches the shell at all
		float marchStart = max(shell.x, 0.0); // clamp in case the camera is inside
		float marchEnd  = (t > 0.0) ? t : shell.y;
		float stepSize = (marchEnd - marchStart) / 16.0;

		for (float i = 0.0; i < 16.0; i++) {
			vec3 samplePos = ro + rd * (marchStart + (i + 0.5) * stepSize);	

			// altitude is distance out from the planet center, so density peaks
			// at the surface and fades to nothing at the top of the shell 
			float altitude = length(samplePos) - PLANET_RADIUS;

			// air is densest at the surface and this out exponentially with height,
			// rayleigh reaching higher than mie. Ozone (.z) is instead a buno
			// at mid altitude, tied to the Rayleigh profile
			// (dimev density + ozone, atmosphere.glsl: 182, 187-188)
			vec3 density;
			density.xy = exp(-altitude / vec2(HEIGHT_RAYLEIGH, HEIGHT_MIE));
			float ozoneDenom = (HEIGHT_OZONE - altitude) / OZONE_FALLOFF;
			density.z = density.x / (ozoneDenom * ozoneDenom + 1.0);
			density *= ATMOSPHERE_DENSITY * stepSize;

			opticalDistance += density;

			// light ray: walk toward the sun and add up the air on the way,
			// We do not test whether the planet blocks the sun. A path
			// grazes a long way thorugh dense low air build hughe optical depth
			// and darkens on its own -> sunset fades smoothly past the
			// terminator onto the night side instead of snapping off
			// Dimev inner light loop, atmoshere.glsl: 197-244)
			vec2 sunShell = sphereIntersection(samplePos, sun, PLANET_RADIUS + ATMOSPHERE_HEIGHT);
			float sunStep = sunShell.y / 8.0;
			vec3 sunDepth = vec3(0.0); 
			for (float j = 0.0; j < 8.0; j++) {
				vec3 sunSample = samplePos + sun * (j + 0.5) * sunStep;
				// max(.,0) if the path clips into the planet, treat as dense surface air
				float sunAltitude = max(length(sunSample) - PLANET_RADIUS, 0.0);
				vec3 sunDensity;
				sunDensity.xy = exp(-sunAltitude / vec2(HEIGHT_RAYLEIGH, HEIGHT_MIE));
				float sunOzoneDenom = (HEIGHT_OZONE - sunAltitude) / OZONE_FALLOFF;
				sunDensity.z = sunDensity.x / (sunOzoneDenom * sunOzoneDenom + 1.0);
				sunDepth += sunDensity * ATMOSPHERE_DENSITY * sunStep;
			}

			// fraction of sunlight reaching this sample and surviving back to
			// the eye: extinction over both parts, per color. The long twilight path
			// eats blue first and leaves red (dimev attn, atmosphere:glsl:248)
			vec3 attentuation = exp(-(C_RAYLEIGH * (opticalDistance.x + sunDepth.x)
						   			+ C_MIE * (opticalDistance.y + sunDepth.y)
						   			+ C_OZONE * (opticalDistance.z + sunDepth.z)));

			scatteredRayleigh += density.x * attentuation;
			scatteredMie += density.y * attentuation;
			
		}

		// opacity of the whole air column used to dim the surface behind it
		// dimev opacity, atmosphere.glsl:260)
 		transmittance = exp(-(C_RAYLEIGH * opticalDistance.x 
					   		+ C_MIE * opticalDistance.y
					   		+ C_OZONE * opticalDistance.z));
    }
	// Each scattering type tinted by its coefficient and aimed by its phase
	// then laid over the surface (already dimmed by the air in front of it)
	// (dimev final terun, atmosphere.glsl:263-267)
	vec3 scatteredLight = phaseR * C_RAYLEIGH * scatteredRayleigh
						+ phaseM * C_MIE * scatteredMie;

    color = color * transmittance + scatteredLight * SUN_INTENSITY;


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
