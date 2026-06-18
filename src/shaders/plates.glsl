// To create continents and mountain ranges, split the sphere into plates
// -> Voronoi partition (every direction belongs to whichever seed point is nearest)
// Needed: Which plate a point is on (to give that a land/ocean level later) and
// how close the point is to a plade edge (where plates collide, mountains rise)
const int PLATE_COUNT = 24;

// tectonic plates seeds spead evenly over the sphere with fibonacci spiral.
// this produces roughly even-sized plates -> add jitter later
// stepping by the golden angle around the sphere drops each seed into the widest
// remaining gap, so plates come out evenly. Seed "i" depends on "i" alone so
// exact same plates regenerate at any resolution.
vec3 fibonacciSeed(int i, int count) {
	float k = (float(i) + 0.5) / float(count); // walk evenly from north pole to south
	float y = 1.0 - 2.0 * k;
	float r = sqrt(max(0.0, 1.0 -y * y));
	float theta = float(i) * 2.39996323; // golden angle
	return vec3(r * cos(theta), y, r * sin(theta));
}

// which plate a direction belongs to and how far it sits form the plate edge.
// closeness = dot(dir, seed), edge distance is the gapp between nearest and
// second-nearest seed: zero on the boundary. Needed to be a pure function of dir
vec2 sphericalVoronoi(vec3 dir, int seedCount) {
	float nearestDot = -2.0;
	float secondDot = -2.0;
	int nearestId = 0;
	for (int i = 0; i < seedCount; i++) {
		float closeness = dot(dir, fibonacciSeed(i, seedCount));
		if (closeness > nearestDot) {
			secondDot = nearestDot;
			nearestDot = closeness;
			nearestId = i;
		} else if (closeness > secondDot) {
			secondDot = closeness;
		}
	}
	return vec2(float(nearestId), nearestDot - secondDot);
}



