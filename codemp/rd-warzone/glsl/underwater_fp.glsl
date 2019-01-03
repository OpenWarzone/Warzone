uniform sampler2D u_TextureMap;

uniform float	u_Time;

varying vec2	vTexCoord0;

#define total_time u_Time

const bool infinite = true;

struct WaveEmitter {
	vec2 mPosition; // = vec2(0.5, 0.5);
	float mAmplitude; // = 0.01;	// factor of final displacement
	float mVelocity; // = 0.05;		// screens per second
	float mWavelength; // = 0.3;	// screens
};

float GetPeriodTime(WaveEmitter emit) {
	return emit.mWavelength / emit.mVelocity;
}

const float emitter_size = 3.0;
WaveEmitter emitter[3];


float GetPhase(vec2 point, WaveEmitter emit, float time) {
	float distance = sqrt( pow(point.x - emit.mPosition.x,2) + pow(point.y - emit.mPosition.y, 2.0) );
	if (!infinite && distance / emit.mVelocity >= time) {
		return 0.0;
	} else {
		return sin((time / GetPeriodTime(emit) - distance / emit.mWavelength) * 2.0 * (2.0 * asin(1.0)));
	}
}

vec2 transformCoord(vec2 orig) {
	vec2 final = orig;
	for(int i = 0; i < int(emitter_size); ++i) {
		vec2 rel = orig - emitter[i].mPosition;
		float fac = GetPhase(orig, emitter[i], total_time) * emitter[i].mAmplitude;
		final += fac * rel;
	}
	return final;
}

vec2 getTransformedCoord ( void )
{
	WaveEmitter emit0;
	emit0.mPosition = vec2(0.1,0.7);
	emit0.mAmplitude = 0.005;
	emit0.mVelocity = 0.06;
	emit0.mWavelength = 0.7;
	emitter[0] = emit0;

	WaveEmitter emit1;
	emit1.mPosition = vec2(0.8,-0.1);
	emit1.mAmplitude = 0.005;
	emit1.mVelocity = 0.07;
	emit1.mWavelength = 0.6;
	emitter[1] = emit1;

	WaveEmitter emit2;
	emit2.mPosition = vec2(1.1,0.9);
	emit2.mAmplitude = 0.005;
	emit2.mVelocity = 0.05;
	emit2.mWavelength = 0.8;
	emitter[2] = emit2;

	vec2 coord = transformCoord(vTexCoord0.st);
	return coord;
}

void main() {
	vec2 coord = getTransformedCoord();
	gl_FragColor = texture2D(u_TextureMap, coord) * 0.6;
}
