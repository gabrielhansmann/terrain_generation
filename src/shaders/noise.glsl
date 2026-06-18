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
// 3D variant of hash(vec2): mapps a lattice corner to a random gradient in [-1,1]^3
// The same corner always returns the same vector which is what lets neighbouring cells
// line up into continuous noise
// This hash function is from https://www.shadertoy.com/view/4djSRW, just mapped frpm
// [0,1] to [-1,1].
vec3 hash3(in vec3 p3) {
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return -1.0 + 2.0 * fract((p3.xxy + p3.yxx)*p3.zyx);
}
// Inigo Quilez version may be faster but less robust, check 
// https://iquilezles.org/articles/gradientnoise/ and
// https://www.shadertoy.com/view/4dffRH

// Analytic gradient noise in3D: value in .x its exact gradient (d/dx,d/dy,d/dz)
// in .yzw. We need the gradient since erosion filter carves gullies down-slope
// so it must know the slope of the base terrain. Reference implementation
// from Inigo Quilez, https://www.shadertoy.com/view/4dffRH, refactored here
// for better readablility.
vec4 noised3(in vec3 p) {
	vec3 cell = floor(p);
	vec3 f = fract(p);

	// Quintic fade: zero 1st and 2nd derivative at 0 and 1
	// fadeSlope = d(fade)/df
	vec3 fade = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	vec3 fadeSlope = 30.0 * f * f * (f * (f - 2.0) + 1.0);

	// random gradient at each of the 8 cube corners
	vec3 gradA = hash3(cell + vec3(0.0, 0.0, 0.0));
    vec3 gradB = hash3(cell + vec3(1.0, 0.0, 0.0));
    vec3 gradC = hash3(cell + vec3(0.0, 1.0, 0.0));
    vec3 gradD = hash3(cell + vec3(1.0, 1.0, 0.0));
    vec3 gradE = hash3(cell + vec3(0.0, 0.0, 1.0));
    vec3 gradF = hash3(cell + vec3(1.0, 0.0, 1.0));
    vec3 gradG = hash3(cell + vec3(0.0, 1.0, 1.0));
    vec3 gradH = hash3(cell + vec3(1.0, 1.0, 1.0));

	// Each corners contribution: its gradient dotted with the vector
	// from the corner to out point. Zero at corner, linear ramp away from it
	float dotA = dot(gradA, f - vec3(0.0, 0.0, 0.0));
    float dotB = dot(gradB, f - vec3(1.0, 0.0, 0.0));
    float dotC = dot(gradC, f - vec3(0.0, 1.0, 0.0));
    float dotD = dot(gradD, f - vec3(1.0, 1.0, 0.0));
    float dotE = dot(gradE, f - vec3(0.0, 0.0, 1.0));
    float dotF = dot(gradF, f - vec3(1.0, 0.0, 1.0));
    float dotG = dot(gradG, f - vec3(0.0, 1.0, 1.0));
    float dotH = dot(gradH, f - vec3(1.0, 1.0, 1.0));

	// Value: trilinear blend of the 8 contributions, weighted by fade
	float value =
        dotA +
        fade.x * (dotB - dotA) +
        fade.y * (dotC - dotA) +
        fade.z * (dotE - dotA) +
        fade.x * fade.y * (dotA - dotB - dotC + dotD) +
        fade.y * fade.z * (dotA - dotC - dotE + dotG) +
        fade.z * fade.x * (dotA - dotB - dotE + dotF) +
        fade.x * fade.y * fade.z * (-dotA + dotB + dotC - dotD + dotE - dotF - dotG + dotH);

	// Gradient = d(value)/dp, by the product rule in two parts
	vec3 gradient =
        // part 1: the same blend, but of the corner GRADIENT vectors.
        gradA +
        fade.x * (gradB - gradA) +
        fade.y * (gradC - gradA) +
        fade.z * (gradE - gradA) +
        fade.x * fade.y * (gradA - gradB - gradC + gradD) +
        fade.y * fade.z * (gradA - gradC - gradE + gradG) +
        fade.z * fade.x * (gradA - gradB - gradE + gradF) +
        fade.x * fade.y * fade.z * (-gradA + gradB + gradC - gradD + gradE - gradF - gradG + gradH) +
        // part 2: how the fade weights themselves shift across the cube.
        fadeSlope * (
            vec3(dotB - dotA, dotC - dotA, dotE - dotA) +
            fade.yzx * vec3(dotA - dotB - dotC + dotD, dotA - dotC - dotE + dotG, dotA - dotB - dotE + dotF) +
            fade.zxy * vec3(dotA - dotB - dotE + dotF, dotA - dotB - dotC + dotD, dotA - dotC - dotE + dotG) +
            fade.yzx * fade.zxy * (-dotA + dotB + dotC - dotD + dotE - dotF - dotG + dotH));

    return vec4(value, gradient);

}


// Used for the height map.
vec3 FractalNoise(vec2 p, float freq, int octaves, float lacunarity, float gain) {
    vec3 n = vec3(0.0);
    float nf = freq;
    float na = 1.0;
    for (int i = 0; i < octaves; i++) {
        n += noised(p * nf) * na * vec3(1.0, nf, nf);
        na *= gain;
        nf *= lacunarity;
    }
    return n;
}


// 3D twin of FactalNoise: sums octaves of noised3 sampled by direction
vec4 FractalNoise3(vec3 p, float freq, int octaves, float lacunarity, float gain) {
    vec4 n = vec4(0.0);
    float nf = freq;
    float na = 1.0;
    for (int i = 0; i < octaves; i++) {
        n += noised3(p * nf) * na * vec4(1.0, nf, nf, nf);
        na *= gain;
        nf *= lacunarity;
    }
    return n;
}

