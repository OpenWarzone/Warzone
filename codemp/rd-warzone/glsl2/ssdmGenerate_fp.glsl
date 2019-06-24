//#define TEST_PARALLAX
//#define TEST_PARALLAX2

uniform sampler2D				u_DiffuseMap;
uniform sampler2D				u_ScreenDepthMap;
uniform sampler2D				u_PositionMap;
uniform sampler2D				u_NormalMap;
uniform sampler2D				u_GlowMap;

uniform vec2					u_Dimensions;

uniform vec4					u_Local1; // DISPLACEMENT_MAPPING_STRENGTH, r_testShaderValue1, r_testShaderValue2, r_testShaderValue3

uniform vec4					u_ViewInfo; // znear, zfar, zfar / znear, fov
uniform vec3					u_ViewOrigin;
uniform vec4					u_PrimaryLightOrigin;

varying vec2					var_TexCoords;

#define DISPLACEMENT_STRENGTH	u_Local1.r

#define znear					u_ViewInfo.r									//camera clipping start
#define zfar					u_ViewInfo.g									//camera clipping end
#define zfar2					u_ViewInfo.a									//camera clipping end

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__

float getDepth(vec2 coord) {
    return texture(u_ScreenDepthMap, coord).r;
}

vec3 getViewPosition(vec2 coord) {
    vec3 pos = vec3((coord.s * 2.0 - 1.0), (coord.t * 2.0 - 1.0) / (u_Dimensions.x/u_Dimensions.y), 1.0);
    return (pos * getDepth(coord));
}

vec3 getViewNormal(vec2 coord) {
	vec3 p0 = getViewPosition(coord);
	vec3 p1 = getViewPosition(coord + vec2(1.0 / u_Dimensions.x, 0.0));
	vec3 p2 = getViewPosition(coord + vec2(0.0, 1.0 / u_Dimensions.y));

	vec3 dx = p1 - p0;
	vec3 dy = p2 - p0;
	return normalize(cross(dy, dx));
}

const vec3 PLUMA_COEFFICIENT = vec3(0.2126, 0.7152, 0.0722);

float lumaForColor(vec3 color)
{
	float luma = dot(color, PLUMA_COEFFICIENT);
	return luma;
}

float plumaAtCoord(vec2 coord) 
{
	vec3 pixel = texture(u_DiffuseMap, coord).rgb;
	return lumaForColor(pixel);
}

float GetDisplacementAtCoord(vec2 coord)
{
#if 1
	if (texture(u_NormalMap, coord).b < 1.0)
	{
		return -1.0;
	}
#endif

#if 0
	vec3 col = texture(u_DiffuseMap, coord).rgb;
	float b = clamp(length(col.rgb) / 3.0), 0.0, 1.0);
	b = clamp(pow(1.0-b, 0.5), 0.0, 1.0);
	return 1.0-b;
#else
#define contLower ( 16.0 / 255.0 )
#define contUpper (255.0 / 156.0 )

	vec2 coord2 = coord;
	coord2.y = 1.0 - coord2.y;
	vec3 gMap = texture(u_GlowMap, coord2).rgb;													// Glow map strength at this pixel
	float invGlowStrength = 1.0 - clamp(max(gMap.r, max(gMap.g, gMap.b)), 0.0, 1.0);

	vec3 pixel = texture(u_DiffuseMap, coord).rgb;
	float luma = lumaForColor(pixel);

	float maxColor = clamp(max(pixel.r, max(pixel.g, pixel.b)), 0.0, 1.0);
	maxColor = clamp(maxColor * 8.0 - 6.0, 0.0, 1.0);

	float displacement = invGlowStrength * ((maxColor + luma) / 2.0);

	// Contrast...
	displacement = clamp((clamp(displacement - contLower, 0.0, 1.0)) * contUpper, 0.0, 1.0);

#ifdef TEST_PARALLAX2
	displacement = clamp(pow(displacement, 0.333) * 1.25, 0.0, 1.0) + displacement;
#endif //TEST_PARALLAX2

	return displacement;
#endif
}

#ifdef TEST_PARALLAX2
#define RAY_STEPS 8
float GetDepth(in vec2 UV, in vec2 Displace)
{
	float Height = 0.0;
	float depth = 1.0;
	float Inc = 1.0 / float(RAY_STEPS);//0.125;

	for( int i = 0; i < RAY_STEPS/*8*/; i++ )
	{
		Height += Inc;
		//float HeightTex = 1.0 - texture(HeightM, UV + Displace * Height);
		float HeightTex = GetDisplacementAtCoord(UV + Displace * Height);

		if (HeightTex == -1.0)
		{
			break;
		}
		
		if ( depth > 0.995 && Height >= HeightTex )
		{
			depth = Height;
		}
	}

	Height=depth;

	for( int i = 0; i < 4; i++ )
	{
		Inc *= 0.5;
		
		//float HeightTex = 1.0 - texture(HeightM, Uv + Displace * Height);
		float HeightTex = GetDisplacementAtCoord(UV + Displace * Height);

		if (HeightTex == -1.0)
		{
			break;
		}
		
		if ( Height >= HeightTex )
		{
			depth = Height;
			Height -= 2.0 * Inc;
		}
		
		Height = Height + Inc;
	}

	return depth;
}

void main(void)
{
	vec3 normal = texture(u_NormalMap, var_TexCoords).rgb;

	if (normal.b < 1.0)
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float depth = getDepth(var_TexCoords);
	float invDepth = clamp((1.0 - depth) /** 2.0 - 1.0*/, 0.0, 1.0);

	if (invDepth <= 0.0)
	{// Sky...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec4 position = texture(u_PositionMap, var_TexCoords);

	if (position.a - 1.0 >= 1024.0)
	{// Sky...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

#if 0
	vec3 V = position.xyz - u_ViewOrigin.xzy;
	//vec3 LightV = normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz);

	vec3 N = DecodeNormal(normal.xy).xzy;

	
	vec3 View = normalize(V);
	float ViewN = -0.5;//dot(View, normalize(u_ViewOrigin.xzy));

	vec2 Displace = ((View * /*0.03*/0.035) / ViewN).xy * pow(invDepth, 8.0);
#else
	/*float material = position.a - 1.0;
	float materialMultiplier = 1.0;

	if (material == MATERIAL_ROCK || material == MATERIAL_STONE || material == MATERIAL_SKYSCRAPER)
	{// Rock gets more displacement...
		materialMultiplier = 3.0;
	}
	else if (material == MATERIAL_TREEBARK)
	{// Rock gets more displacement...
		materialMultiplier = 1.5;
	}*/

	vec3 N = DecodeNormal(normal.xy).xzy;
	vec2 Displace = /*N.xy*/vec2(dot(N.x, N.z), N.y) * vec2((-DISPLACEMENT_STRENGTH /** materialMultiplier*/) / u_Dimensions) * pow(invDepth, 8.0);
	//Displace.x *= -1.0;
#endif

	float Ray = GetDepth(var_TexCoords, Displace);

	vec2 coord2 = var_TexCoords;
	coord2.y = 1.0 - coord2.y;
	vec3 gMap = texture(u_GlowMap, coord2).rgb;													// Glow map strength at this pixel
	float invGlowStrength = 1.0 - clamp(max(gMap.r, max(gMap.g, gMap.b)), 0.0, 1.0);

	Ray *= invGlowStrength;

	/*
	vec2 NewUv = (var_TexCoords + Displace * Ray);

	
	vec2 Texture = texture(u_DiffuseMap, NewUv);
	vec2 NormalMap = texture(u_NormalMap, NewUv);
	NormalMap = normalize(NormalMap);
	float Normal = clamp(dot(reflect(-View, NormalMap), LightV), 0.0, 1.0);
	Normal = pow(Normal, SpecularPow) + saturate(dot(NormalMap, LightV));
	float PixelLight = 1.0 - clamp(dot(IN.Attenuation, IN.Attenuation), 0.0, 1.0);
	vec3 Light = PixelLight * LightColor;
	return vec4(Texture * ((Normal * Light) + Ambient), Texture.w * Alpha);
	*/

	
	//gl_FragColor = vec4(Ray, norm.x * 0.5 + 0.5, norm.y * 0.5 + 0.5, 1.0);
	gl_FragColor = vec4(Ray, Displace.x * 0.5 + 0.5, Displace.y * 0.5 + 0.5, 1.0);

}

#elif defined(TEST_PARALLAX)
/*
uniform int MinSamples; //! slider[1, 1, 100]
uniform int MaxSamples; //! slider[20, 20, 256]
uniform bool UseShadow; //! checkbox[true]
uniform float SteepHeightScale; //! slider[0.005, 0.05, 0.1]
uniform float ShadowOffset; //! slider[0.01, 0.05, 0.5]
*/

const int linear_steps = 10;

int MinSamples = 1; //! slider[1, 1, 100]
int MaxSamples = int(u_Local1.a); //! slider[20, 20, 256]
bool UseShadow = true;

float SteepHeightScale = u_Local1.g; //! slider[0.005, 0.05, 0.1]
float ShadowOffset = u_Local1.b; //! slider[0.01, 0.05, 0.5]

#define SQR(x) ((x) * (x))

vec2 raymarch(vec2 startPos, vec3 dir) {
	// Compute initial parallax displacement direction:
	vec2 parallaxDirection = normalize(dir.xy);

	// The length of this vector determines the
	// furthest amount of displacement:
	float parallaxLength = sqrt(1.0 - SQR(dir.z));
	parallaxLength /= dir.z;

	// Compute the actual reverse parallax displacement vector:
	vec2 parallaxOffset = parallaxDirection * parallaxLength;

	// Need to scale the amount of displacement to account
	// for different height ranges in height maps.
	parallaxOffset *= SteepHeightScale;

	// corrected for tangent space. Normal is always z=1 in TS and
	// v.viewdir is in tangent space as well...
	int numSteps = int(mix(MaxSamples, MinSamples, dir.z));

	float currHeight = 0.0;
	float stepSize = 1.0 / float(numSteps);
	int stepIndex = 0;
	vec2 texCurrentOffset = startPos;
	vec2 texOffsetPerStep = stepSize * parallaxOffset;

	vec2 resultTexPos = vec2(texCurrentOffset - (texOffsetPerStep * numSteps));

	float prevHeight = 1.0;
	float currRayDist = 1.0;

	while (stepIndex < numSteps) {
		// Determine where along our ray we currently are.
		currRayDist -= stepSize;
		texCurrentOffset -= texOffsetPerStep;
		currHeight = GetDisplacementAtCoord(texCurrentOffset).r;

		// Because we're using heights in the [0..1] range
		// and the ray is defined in terms of [0..1] scanning
		// from top-bottom we can simply compare the surface
		// height against the current ray distance.
		if (currHeight >= currRayDist) {
			// Push the counter above the threshold so that
			// we exit the loop on the next iteration
			stepIndex = numSteps + 1;

			// We now know the location along the ray of the first
			// point *BELOW* the surface and the previous point
			// *ABOVE* the surface:
			float rayDistAbove = currRayDist + stepSize;
			float rayDistBelow = currRayDist;

			// We also know the height of the surface before and
			// after we intersected it:
			float surfHeightBefore = prevHeight;
			float surfHeightAfter = currHeight;

			float numerator = rayDistAbove - surfHeightBefore;
			float denominator = (surfHeightAfter - surfHeightBefore)
					- (rayDistBelow - rayDistAbove);

			// As the angle between the view direction and the
			// surface becomes closer to parallel (e.g. grazing
			// view angles) the denominator will tend towards zero.
			// When computing the final ray length we'll
			// get a divide-by-zero and bad things happen.
			float x = 0.0;

			if (abs(denominator) > 1e-5) {
				x = numerator / denominator;
			}

			// Now that we've found the position along the ray
			// that indicates where the true intersection exists
			// we can translate this into a texture coordinate
			// - the intended output of this utility function.

			resultTexPos = mix(texCurrentOffset + texOffsetPerStep, texCurrentOffset, x);
		} else {
			++stepIndex;
			prevHeight = currHeight;
		}
	}

	return resultTexPos;
}

float raymarchShadow(vec2 startPos, vec3 dir) {
	vec2 parallaxDirection = normalize(dir.xy);

	float parallaxLength = sqrt(1.0 - SQR(dir.z));
	parallaxLength /= dir.z;

	vec2 parallaxOffset = parallaxDirection * parallaxLength;
	parallaxOffset *= SteepHeightScale;

	int numSteps = int(mix(MaxSamples, MinSamples, dir.z));

	float currHeight = 0.0;
	float stepSize = 1.0 / float(numSteps);
	int stepIndex = 0;

	vec2 texCurrentOffset = startPos;
	vec2 texOffsetPerStep = stepSize * parallaxOffset;

	float initialHeight = GetDisplacementAtCoord(startPos).r + ShadowOffset;

	while (stepIndex < numSteps) {
		texCurrentOffset += texOffsetPerStep;

		float rayHeight = mix(initialHeight, 1.0, float(stepIndex / numSteps));

		currHeight = GetDisplacementAtCoord(texCurrentOffset).r;

		if (currHeight > rayHeight) {
			// ray has gone below the height of the surface, therefore
			// this pixel is occluded...
			return 0.0;
		}

		++stepIndex;
	}

	return 1.0;
}

vec3 steepParallax(vec3 V, vec3 L, vec3 N, vec2 T) {
	vec3 result = vec3(1.0);
	float shadow = 1.0;
	vec2 T2 = raymarch(T, -V);

	if (UseShadow) {
		shadow = raymarchShadow(T, -L);
	}

	return vec3(T2-T, shadow);
}

void main(void)
{
	if (texture(u_NormalMap, var_TexCoords).b < 1.0)
	{// An if based on output from a texture, but seems to increase FPS a little anyway...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 position = texture(u_PositionMap, var_TexCoords).xyz;

	vec3 V = normalize(position.xyz - u_ViewOrigin.xyz).xyz;
	vec3 L = normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz).xyz;

	vec3 N = DecodeNormal(textureLod(u_NormalMap, var_TexCoords, 0.0).xy);
	//vec3 N = getViewNormal(var_TexCoords);
	//vec3 tangent = TangentFromNormal( norm.xyz );
	//vec3 bitangent = normalize( cross(norm.xyz, tangent) );
	//mat3 tangentToWorld = mat3(tangent.xyz, bitangent.xyz, norm.xyz);

	vec3 parallax = steepParallax(V, L, N, var_TexCoords);

	gl_FragColor = vec4(parallax.xy*0.5+0.5, parallax.z, 1.0);
}

#else //!defined(TEST_PARALLAX)
float ReliefMapping(vec2 dp, vec2 ds, float origDepth, float materialMultiplier)
{
	//return clamp(GetDisplacementAtCoord(dp + ds), 0.0, 1.0);

#ifdef __HQ_PARALLAX__
	int linear_steps = 10 * int(materialMultiplier);
	int binary_steps = 5 * int(materialMultiplier);
#else //!__HQ_PARALLAX__
	const int linear_steps = 10;// 4;// 5;// 10;
	const int binary_steps = 5;// 2;// 5;
#endif //__HQ_PARALLAX__
	float size = 1.0 / linear_steps;
	float depth = 1.0;
	float best_depth = 1.0;
	float stepsDone = 0;

	for (int i = 0; i < linear_steps - 1; ++i) 
	{
		stepsDone += 1.0;
		depth -= size;
		float t = GetDisplacementAtCoord(dp + ds * depth);
		if (t == -1.0) break;
		if (depth >= t)
			best_depth = depth;
	}

	depth = best_depth - size;

	for (int i = 0; i < binary_steps; ++i) 
	{
		size *= 0.5;
		float t = GetDisplacementAtCoord(dp + ds * depth);
		if (t == -1.0) break;
		if (depth >= t) {
			best_depth = depth;
			depth -= 2 * size;
		}
		depth += size;
	}

	float finished = (stepsDone / linear_steps);
	return clamp(best_depth, 0.0, 1.0) * finished;
}

void main(void)
{
	if (texture(u_NormalMap, var_TexCoords).b < 1.0)
	{// An if based on output from a texture, but seems to increase FPS a little anyway...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float depth = getDepth(var_TexCoords);
	float invDepth = clamp((1.0 - depth) * 2.0 - 1.0, 0.0, 1.0);

	if (invDepth <= 0.0)
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 norm = getViewNormal(var_TexCoords);
	float material = texture(u_PositionMap, var_TexCoords).a - 1.0;
	
#if 1
	float materialMultiplier = 1.0;

	if (material == MATERIAL_ROCK || material == MATERIAL_STONE || material == MATERIAL_SKYSCRAPER)
	{// Rock gets more displacement...
		materialMultiplier = 3.0;
	}
	else if (material == MATERIAL_TREEBARK)
	{// Rock gets more displacement...
		materialMultiplier = 1.5;
	}
	//const float materialMultiplier = 1.0;

	vec2 coord2 = var_TexCoords;
	coord2.y = 1.0 - coord2.y;
	vec3 gMap = texture(u_GlowMap, coord2).rgb;													// Glow map strength at this pixel
	float invGlowStrength = 1.0 - clamp(max(gMap.r, max(gMap.g, gMap.b)), 0.0, 1.0);

	vec2 ParallaxXY = norm.xy * vec2((-DISPLACEMENT_STRENGTH * materialMultiplier) / u_Dimensions) * invDepth;
	float displacement = invGlowStrength * ReliefMapping(var_TexCoords, ParallaxXY, depth, materialMultiplier);
#elif 0
	//norm.xyz = vec3(var_TexCoords.x * 2.0 - 1.0, var_TexCoords.y, 1.0);
	norm.xyz = vec3(dot(var_TexCoords.x * 2.0 - 1.0, -norm.x), dot(var_TexCoords.y * 2.0 - 1.0, -norm.y), 1.0);
	float displacement = GetDisplacementAtCoord(var_TexCoords);

#else
	float displacement = GetDisplacementAtCoord(var_TexCoords);
#endif
	
	gl_FragColor = vec4(displacement, norm.x * 0.5 + 0.5, norm.y * 0.5 + 0.5, 1.0);
}
#endif //TEST_PARALLAX

