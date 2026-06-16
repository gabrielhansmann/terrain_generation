/*
====================================================================================

See Buffer A for information on the erosion technique.

====================================================================================
*/

// Settings are injected by the host app via #define.
// Defaults (commented out):
// #define ANIMATE_PARAMETERS 1

#define PI 3.14159265358979

void AnimateTo(inout float current, float target, float time) {
    #if ANIMATE_PARAMETERS
        current = mix(current, target, smoothstep(0.0, 1.0, time));
    #endif
}

void AnimateWaveTo(inout float current, float target, float time) {
    #if ANIMATE_PARAMETERS
        time = clamp(time, 0.0, 1.0);
        current = mix(current, target, 0.5 - 0.5 * cos(3.0 * time * PI));
    #endif
}

void AnimateLoHi(inout float current, float lo, float hi, float time) {
    #if ANIMATE_PARAMETERS
        float og = current;
        current = mix(current, lo, smoothstep(0.0, 1.0, time));
        current = mix(current, hi, smoothstep(0.0, 1.0, time - 2.0));
        current = mix(current, og, smoothstep(0.0, 1.0, time - 4.0));
    #endif
}


// -----------------------------------------------------------------------------
// Debug options
// -----------------------------------------------------------------------------

// Debug options (injected)
// #define GREYSCALE 0
// #define SHOW_RIDGEMAP 0
// #define SHOW_DIFFUSE 0
// #define SHOW_NORMALS 0
// #define COMPARISON_SLIDER 0
// #define SHOW_BUFFER 0
// #define SHOW_BUFFER_NR 0


// -----------------------------------------------------------------------------
// Renderer settings
// -----------------------------------------------------------------------------

// Renderer settings (injected)
// #define FIXED_SUN 1
// #define WATER 1
// #define WATER_HEIGHT (0.36 + 0.1 * (smoothstep(54.0, 60.0, mod(iTime, 120.0)) - smoothstep(114.0, 120.0, mod(iTime, 120.0))))
// #define FOG_HEIGHT 0.465
// #define GRASS_HEIGHT 0.465
// #define DRAINAGE 1
// #define DRAINAGE_WIDTH 0.3
// #define TREES 1
// #define DETAIL_TEXTURE 1
// #define RAYMARCH_QUALITY 2.0


// -----------------------------------------------------------------------------
// Camera settings
// -----------------------------------------------------------------------------

// Camera settings (injected)
// #define CAMERA_MOUSE_CONTROL 1
// #define LOW_ANGLE (mod(iTime, 240.0) >= 120.0)
// #define CAMERA_DIST (LOW_ANGLE ? 1.5 : 3.25)
// #define CAMERA_FOV (LOW_ANGLE ? 20.0 : 11.0)
// #define CAMERA_ANGLE (LOW_ANGLE ? 0.25 : -0.45)
// #define CAMERA_ELEVATION (LOW_ANGLE ? -0.35 : -0.43)
// #define CAMERA_LOOKAT vec3(0.0, 0.40, 0.0)
// #define TIME_SCROLL_OFFSET vec2(cos(iTime / 60.0 * 2.0 * PI) * 2.0, -sin(iTime / 60.0 * 2.0 * PI) * 0.1)
// #define TIME_CAM_SPIN (LOW_ANGLE ? 0.0 : 1.0 / 60.0)
// #define TIME_CAM_WOBBLE 0.0


// -----------------------------------------------------------------------------
// Material and color values
// -----------------------------------------------------------------------------

// Materials

#define M_GROUND 0
#define M_STRATA 1
#define M_WATER  2

// Colors

// Colors (injected)
// #define CLIFF_COLOR    vec3(0.22, 0.2, 0.2)
// #define DIRT_COLOR     vec3(0.6, 0.5, 0.4)
// #define TREE_COLOR     vec3(0.12, 0.26, 0.1)
// #define GRASS_COLOR1   vec3(0.15, 0.3, 0.1)
// #define GRASS_COLOR2   vec3(0.4, 0.5, 0.2)
// #define SAND_COLOR     vec3(0.8, 0.7, 0.6)
// #define WATER_COLOR vec3(0.0, 0.05, 0.1)
// #define WATER_SHORE_COLOR vec3(0.0, 0.25, 0.25)
// #define SUN_COLOR (vec3(1.0, 0.98, 0.95) * 2.0)
// #define AMBIENT_COLOR (vec3(0.3, 0.5, 0.7) * 0.1)


// -----------------------------------------------------------------------------
// Buffer size functionality
// -----------------------------------------------------------------------------

// Limit the work area of Buffer A/B to speed things up
// #define BUFFER_SIZE vec2(min(min(iResolution.x, iResolution.y), 1080.0))
//#define BUFFER_SIZE vec2(min(min(iResolution.x, iResolution.y), 768.0))
#define BUFFER_SIZE vec2(1024.0)
#define PLANET_RADIUS 1.0
#define HEIGHT_SCALE 0.08
#define ATMOSPHERE_HEIGHT 0.09 // how far the atmosphere is above the surface in planet radii
// sacle heights: the altitude over which each scattering type thins out. 
// Mie (haze, dust) hugs the surface; Rayleight (the blue of the sky) reaches higher.
// ratios taken from dimev's HEIGHT_RAY=8e3 / HEIGHT_MIE=1.2e3 over a 100e3-thick
// atmosphere (atmosphere.glsl:72-73), rescaled to our shell.
// -> https://github.com/Dimev/atmospheric-scattering-explained / https://www.shadertoy.com/view/wlBXWK
#define HEIGHT_RAYLEIGH (ATMOSPHERE_HEIGHT * 0.08)
#define HEIGHT_MIE (ATMOSPHERE_HEIGHT * 0.012)
// The ozon layer sits in a band at mid altitude, not ground. Peak and width as Dimevs
// 30e3 / 4e3 over a 100e3 atmosphere (atmosphere.glsl:74-75)
#define HEIGHT_OZONE (ATMOSPHERE_HEIGHT * 0.3)
#define OZONE_FALLOFF (ATMOSPHERE_HEIGHT * 0.04)
// particle-count multiplier. World is measured in planet radii (~1) but the 
// scattering coefficients are tuned for metres, so this rescales the air into
// a visible thickness. The single know for "how hazy the planet looks"
#define ATMOSPHERE_DENSITY 3e6
#define SUN_INTENSITY 25.0
// sun disc geometry in radians. SIZE is the angular radius of the bright core,
// SOFTNESS is the width of the blurred rim added outside it
#define SUN_DISC_SIZE 0.02
#define SUN_DISC_SOFTNESS 0.3
#define DISCARD_MAP (fragCoord.x >= BUFFER_SIZE.x || fragCoord.y >= BUFFER_SIZE.y)
#define TIME_SCROLL_OFFSET_INT (round(TIME_SCROLL_OFFSET * BUFFER_SIZE) / BUFFER_SIZE)
#define TIME_SCROLL_OFFSET_FRAC (TIME_SCROLL_OFFSET - TIME_SCROLL_OFFSET_INT)


// -----------------------------------------------------------------------------
// Misc utility functions
// -----------------------------------------------------------------------------

#define DEG_TO_RAD (PI / 180.0)
#define clamp01(x) clamp(x, 0.0, 1.0)
#define sq(x) (x*x)

vec2 hash(in vec2 x) {
    const vec2 k = vec2(0.3183099, 0.3678794);
    x = x * k + k.yx;
    return -1.0 + 2.0 * fract(16.0 * k * fract(x.x * x.y * (x.x + x.y)));
}


// Returns gradient noise (in x) and its derivatives (in yz).
// From https://www.shadertoy.com/view/XdXBRH
vec3 noised(in vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    vec2 du = 30.0 * f * f * (f * (f - 2.0) + 1.0); 
    
    vec2 ga = hash(i + vec2(0.0, 0.0));
    vec2 gb = hash(i + vec2(1.0, 0.0));
    vec2 gc = hash(i + vec2(0.0, 1.0));
    vec2 gd = hash(i + vec2(1.0, 1.0));
    
    float va = dot(ga, f - vec2(0.0, 0.0));
    float vb = dot(gb, f - vec2(1.0, 0.0));
    float vc = dot(gc, f - vec2(0.0, 1.0));
    float vd = dot(gd, f - vec2(1.0, 1.0));

    return vec3(va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd),
        ga + u.x * (gb - ga) + u.y * (gc - ga) + u.x * u.y * (ga - gb - gc + gd) +
        du * (u.yx * (va - vb - vc + vd) + vec2(vb, vc) - va));
}

// From https://iquilezles.org/articles/intersectors
vec2 boxIntersection(in vec3 ro, in vec3 rd, vec3 boxSize, out vec3 outNormal) {
    vec3 m = 1.0 / rd; // can precompute if traversing a set of aligned boxes
    vec3 n = m * ro;   // can precompute if traversing a set of aligned boxes
    vec3 k = abs(m) * boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);
    if (tN > tF || tF < 0.0)
        return vec2(-1.0); // no intersection
    outNormal = -sign(rd) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz);
    return vec2(tN, tF);
}
// ray vs a sphere centered at the origin -> returns the near and far ray
// parameters where the ray crosses the spehere: a negative .y means the sphere
// is behind the ray or missed entirely. Assumes rd is normalized
vec2 sphereIntersection(vec3 ro, vec3 rd, float radius) {
	// ray param of the point that passes closest to the spere center
	float tClosest = -dot(ro, rd);

	float distSq = dot(ro, ro) - tClosest * tClosest;
	float radiusSq = radius * radius;
	if (distSq > radiusSq) return vec2(-1.0); // ray misses the sphere

	// ray cuts a line thorugh the sphere and get half its length
	// Chord = line that cuts thorugh sphere, connecting two points
	float halfChord = sqrt(radiusSq - distSq);
	return vec2(tClosest - halfChord, tClosest + halfChord);
}

// ==========================================================================================
// Set up camera
// ==========================================================================================

// From https://www.shadertoy.com/view/XsB3Rm
vec3 CameraRay(float fov, vec2 size, vec2 pos) {
    vec2 xy = pos - size * 0.5;
    float cot_half_fov = tan((90.0 - fov * 0.5) * DEG_TO_RAD);    
    float z = size.y * 0.5 * cot_half_fov;
    return normalize(vec3(xy, -z));
}
mat3 CameraRotation(vec2 angle) {
    vec2 c = cos(angle);
    vec2 s = sin(angle);
    return mat3(
        c.y      ,  0.0, -s.y,
        s.y * s.x,  c.x,  c.y * s.x,
        s.y * c.x, -s.x,  c.y * c.x
    );
}

void GetRay(out vec3 ro, out vec3 rd, float iTime, vec4 iMouse, vec3 iResolution, vec2 fragCoord, float debugWidth) {
    float iRevolution = iTime * 2.0 * PI;
    vec2 cameraAngle = vec2(
        iRevolution * TIME_CAM_SPIN
            + sin(iRevolution / 6.0) * TIME_CAM_WOBBLE * 6.0
            + CAMERA_ANGLE * 2.0 * PI,
        CAMERA_ELEVATION);
    float cameraDistance = CAMERA_DIST;
    
    // Intro animation
    //cameraAngle.x -= exp(-iTime * 5.0) * 4.0;
    //cameraDistance += exp(-iTime * 5.0) * 5.0;
    
    #if CAMERA_MOUSE_CONTROL
        // Control camera orbit position with mouse when held down.
        if (iMouse.z > 0.5) {
            vec2 mouse = iMouse.xy / iResolution.xy;
            mouse.y = clamp01(mix(mouse.y, 0.5, -1.0));
            cameraAngle = (mouse - vec2(0.5, 1.0)) * vec2(-PI * 2.0, PI * 0.5);
        }
    #endif

    mat3 rot = CameraRotation(cameraAngle.yx);
    
    // Ray direction.
    rd = CameraRay(CAMERA_FOV, iResolution.xy, fragCoord.xy - vec2(debugWidth / 2.0, 0.0));
    rd = rot * rd;
    
    // Ray origin.
    ro = CAMERA_LOOKAT + rot * vec3(0, 0, cameraDistance);
}

vec3 GetMouseWorldPos(vec3 ro, vec3 rd, float planeHeight) {
    return ro + rd * (planeHeight - ro.y) / rd.y;
}

vec3 SkyColor(vec3 rd, vec3 sun) {
    float costh = dot(rd, sun);
    return AMBIENT_COLOR * PI * (1.0 - abs(costh) * 0.8);
}

vec3 Tonemap_ACES(vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}


// -----------------------------------------------------------------------------
// BRDF functionality from https://www.shadertoy.com/view/XlKSDR
// -----------------------------------------------------------------------------

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float D_GGX(float linearRoughness, float NoH, const vec3 h) {
    // Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
    float oneMinusNoHSquared = 1.0 - NoH * NoH;
    float a = NoH * linearRoughness;
    float k = linearRoughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return d;
}

float V_SmithGGXCorrelated(float linearRoughness, float NoV, float NoL) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = linearRoughness * linearRoughness;
    float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return 0.5 / (GGXV + GGXL);
}

vec3 F_Schlick(const vec3 f0, float VoH) {
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
    return f0 + (vec3(1.0) - f0) * pow5(1.0 - VoH);
}

float F_Schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float Fd_Burley(float linearRoughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * linearRoughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter  = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

vec3 Shade(vec3 diffuse, vec3 f0, float smoothness, vec3 n, vec3 v, vec3 l, vec3 lc) {
    vec3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp01(dot(n, l));
    float NoH = clamp01(dot(n, h));
    float LoH = clamp01(dot(l, h));

    float roughness = 1.0 - smoothness;
    float linearRoughness = roughness * roughness;
    float D = D_GGX(linearRoughness, NoH, h);
    float V = V_SmithGGXCorrelated(linearRoughness, NoV, NoL);
    vec3 F = F_Schlick(f0, LoH);
    vec3 Fr = (D * V) * F;

    vec3 Fd = diffuse * Fd_Burley(linearRoughness, NoV, NoL, LoH);

    return (Fd + Fr) * lc * NoL;
}

// ------------------------------------------------------------------------------
// Atmosphere
// ------------------------------------------------------------------------------

#define C_RAYLEIGH (vec3(5.802, 13.558, 33.100) * 1e-6)
#define C_MIE (vec3(3.996,  3.996,  3.996) * 1e-6)
#define C_OZONE (vec3(2.04, 4.97, 0.195) * 1e-5)

float PhaseRayleigh(float costh) {
	return 3.0 * (1.0 + costh * costh) / (16.0 * PI);
}

float PhaseMie(float costh, float g) {
	g = min(g, 0.9381);
	float k = 1.55 * g - 0.55 * g * g * g;
	float kcosth = k * costh;
	return (1.0 - k * k) / ((4.0 * PI) * (1.0 - kcosth) * (1.0 - kcosth));
}


// ------------------------------------------------------------------------------
// Packing
// ------------------------------------------------------------------------------

// Some methods to package colours from a vec4 (0-1) into a single 32-bit float.
float pack4(in vec4 rgba) {
    lowp int red = clamp(int(rgba.r * 255.0), 0, 255);
    lowp int green = clamp(int(rgba.g * 255.0), 0, 255);
    lowp int blue = clamp(int(rgba.b * 255.0), 0, 255);
    lowp int alpha = clamp(int(rgba.a * 255.0), 0, 255);

    return intBitsToFloat((red << 24) | (green << 16) | (blue << 8) | alpha);
}

vec4 unpack4(in float col) {
    highp int val = floatBitsToInt(col);

    return vec4(
        float((val >> 24) & 255) / 255.0,
        float((val >> 16) & 255) / 255.0,
        float((val >> 8) & 255) / 255.0,
        float(val & 255) / 255.0
    );
}
