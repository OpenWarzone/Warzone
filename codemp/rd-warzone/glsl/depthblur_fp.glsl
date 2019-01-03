uniform sampler2D u_ScreenImageMap;
uniform sampler2D u_ScreenDepthMap;

uniform vec4   u_ViewInfo; // zfar / znear, zfar
varying vec2   var_ScreenTex;

const float gauss[4] = float[4](0.40, 0.24, 0.054, 0.0044);

#define GAUSS_SIZE 4

float getLinearDepth(sampler2D depthMap, const vec2 tex, const float zFarDivZNear)
{
		float sampleZDivW = textureLod(depthMap, tex, 0.0).r;
		return 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);
}

vec4 depthGaussian1D(sampler2D imageMap, sampler2D depthMap, vec2 tex, float zFarDivZNear, float zFar)
{
	const float scale = 1.0 / 256.0;

#if defined(USE_HORIZONTAL_BLUR)
    vec2 direction = vec2(1.0, 0.0) * scale;
#else // if defined(USE_VERTICAL_BLUR)
	vec2 direction = vec2(0.0, 1.0) * scale;
#endif
	
	float depthCenter = zFar * getLinearDepth(depthMap, tex, zFarDivZNear);
	vec2 centerSlope = vec2(dFdx(depthCenter), dFdy(depthCenter)) / vec2(dFdx(tex.x), dFdy(tex.y));
		
	vec4 result = textureLod(imageMap, tex, 0.0) * gauss[0];
	float total = gauss[0];

	int i, j;
	for (i = 0; i < 2; i++)
	{
		for (j = 1; j < GAUSS_SIZE; j++)
		{
			vec2 offset = direction * j;
			float depthSample = zFar * getLinearDepth(depthMap, tex + offset, zFarDivZNear);
			float depthExpected = depthCenter + dot(centerSlope, offset);
			if(abs(depthSample - depthExpected) < 5.0)
			{
				result += textureLod(imageMap, tex + offset, 0.0) * gauss[j];
				total += gauss[j];
			}
		}
		
		direction = -direction;
	}	
		
	return result / total;
}

void main()
{		
	gl_FragColor = depthGaussian1D(u_ScreenImageMap, u_ScreenDepthMap, var_ScreenTex, u_ViewInfo.x, u_ViewInfo.y);
}
