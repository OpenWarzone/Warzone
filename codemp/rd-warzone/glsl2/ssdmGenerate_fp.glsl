//#define TEST_PARALLAX

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

	return displacement;
}

#ifndef TEST_PARALLAX
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
#else //TEST_PARALLAX
#define parallaxScale u_Local1.g

vec2 parallaxMapping(in vec3 V, in vec2 T, out float parallaxHeight)
{
   // determine optimal number of layers
   const float minLayers = 10;
   const float maxLayers = 15;
   float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), V)));

   // height of each layer
   float layerHeight = 1.0 / numLayers;
   // current depth of the layer
   float curLayerHeight = 0;
   // shift of texture coordinates for each layer
   vec2 dtex = parallaxScale * V.xy / V.z / numLayers;

   // current texture coordinates
   vec2 currentTextureCoords = T;

   // depth from heightmap
   float heightFromTexture = GetDisplacementAtCoord(currentTextureCoords).r;

   float layer = 0.0;

   // while point is above the surface
   while(heightFromTexture > curLayerHeight) 
   {
      // to the next layer
      curLayerHeight += layerHeight; 
      // shift of texture coordinates
      currentTextureCoords -= dtex;
      // new depth from heightmap
      heightFromTexture = GetDisplacementAtCoord(currentTextureCoords).r;

	  layer += 1.0;
	  if (layer >= maxLayers) break;
   }

   ///////////////////////////////////////////////////////////

   // previous texture coordinates
   vec2 texStep	= dtex;
   vec2 prevTCoords = currentTextureCoords + texStep;

   // heights for linear interpolation
   float nextH	= heightFromTexture - curLayerHeight;
   float prevH	= GetDisplacementAtCoord(prevTCoords).r - curLayerHeight + layerHeight;

   // proportions for linear interpolation
   float weight = nextH / (nextH - prevH);

   // interpolation of texture coordinates
   vec2 finalTexCoords = prevTCoords * weight + currentTextureCoords * (1.0-weight);

   // interpolation of depth values
   parallaxHeight = curLayerHeight + prevH * weight + nextH * (1.0 - weight);

   // return result
   return finalTexCoords;
}

float parallaxSoftShadowMultiplier(in vec3 L, in vec2 initialTexCoord, in float initialHeight)
{
   float shadowMultiplier = 1;

   const float minLayers = 15;
   const float maxLayers = 30;

   // calculate lighting only for surface oriented to the light source
   if(dot(vec3(0, 0, 1), L) > 0)
   {
      // calculate initial parameters
      float numSamplesUnderSurface	= 0;
      shadowMultiplier	= 0;
      float numLayers	= mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), L)));
      float layerHeight	= initialHeight / numLayers;
      vec2 texStep	= parallaxScale * L.xy / L.z / numLayers;

      // current parameters
      float currentLayerHeight	= initialHeight - layerHeight;
      vec2 currentTextureCoords	= initialTexCoord + texStep;
      float heightFromTexture	= GetDisplacementAtCoord(currentTextureCoords).r;
      int stepIndex	= 1;

	  float layer = 0.0;

      // while point is below depth 0.0 )
      while(currentLayerHeight > 0)
      {
         // if point is under the surface
         if(heightFromTexture < currentLayerHeight)
         {
            // calculate partial shadowing factor
            numSamplesUnderSurface	+= 1;
            float newShadowMultiplier	= (currentLayerHeight - heightFromTexture) * (1.0 - stepIndex / numLayers);
            shadowMultiplier	= max(shadowMultiplier, newShadowMultiplier);
         }

         // offset to the next layer
         stepIndex	+= 1;
         currentLayerHeight	-= layerHeight;
         currentTextureCoords	+= texStep;
         heightFromTexture	= GetDisplacementAtCoord(currentTextureCoords).r;

		layer += 1.0;
		if (layer >= maxLayers) break;
      }

      // Shadowing factor should be 1 if there were no points under the surface
      if(numSamplesUnderSurface < 1)
      {
         shadowMultiplier = 1;
      }
      else
      {
         shadowMultiplier = 1.0 - shadowMultiplier;
      }
   }
   return shadowMultiplier;
}

vec3 TangentFromNormal ( vec3 normal )
{
	vec3 tangent;
	vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0)); 
	vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0)); 

	if( length(c1) > length(c2) )
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	return normalize(tangent);
}
#endif //TEST_PARALLAX

void main(void)
{
	if (texture(u_NormalMap, var_TexCoords).b < 1.0)
	{// An if based on output from a texture, but seems to increase FPS a little anyway...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

#ifndef TEST_PARALLAX

	float depth = getDepth(var_TexCoords);
	float invDepth = clamp((1.0 - depth) * 2.0 - 1.0, 0.0, 1.0);

	if (invDepth <= 0.0)
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 norm = getViewNormal(var_TexCoords);
	float material = texture(u_PositionMap, var_TexCoords).a - 1.0;
	
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

#if 1
	vec2 coord2 = var_TexCoords;
	coord2.y = 1.0 - coord2.y;
	vec3 gMap = texture(u_GlowMap, coord2).rgb;													// Glow map strength at this pixel
	float invGlowStrength = 1.0 - clamp(max(gMap.r, max(gMap.g, gMap.b)), 0.0, 1.0);

	vec2 ParallaxXY = norm.xy * vec2((-DISPLACEMENT_STRENGTH * materialMultiplier) / u_Dimensions) * invDepth;
	float displacement = invGlowStrength * ReliefMapping(var_TexCoords, ParallaxXY, depth, materialMultiplier);
#else
	float displacement = GetDisplacementAtCoord(var_TexCoords);
#endif
	
	gl_FragColor = vec4(displacement, norm.x * 0.5 + 0.5, norm.y * 0.5 + 0.5, 1.0);

#else //TEST_PARALLAX

	// normalize vectors after vertex shader
	//vec3 V = normalize(o_toCameraInTangentSpace);
	//vec3 L = normalize(o_toLightInTangentSpace);

	vec3 position = texture(u_PositionMap, var_TexCoords).xyz;

	vec3 V = normalize(u_ViewOrigin.xyz - position.xyz).xzy;
	vec3 L = normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz).xzy;

	vec3 norm = DecodeNormal(textureLod(u_NormalMap, var_TexCoords, 0.0).xy);
	vec3 tangent = TangentFromNormal( norm.xzy );
	vec3 bitangent = normalize( cross(norm.xzy, tangent) );
	//mat3 tangentToWorld = mat3(tangent.xzy, bitangent.xzy, norm.xzy);

	V = vec3(
		dot(V, norm),
		dot(V, tangent),
		dot(V, bitangent)
	);

	L = vec3(
		dot(L, norm),
		dot(L, tangent),
		dot(L, bitangent)
	);

	// get new texture coordinates from Parallax Mapping
	float parallaxHeight;
	vec2 T = parallaxMapping(V, var_TexCoords, parallaxHeight);

	// get self-shadowing factor for elements of parallax
	float shadowMultiplier = parallaxSoftShadowMultiplier(L, T, parallaxHeight - 0.05);

	// calculate lighting
	//resultingColor = normalMappingLighting(T, L, V, shadowMultiplier);

	gl_FragColor = vec4(T*0.5+0.5, shadowMultiplier, 1.0);

#endif //TEST_PARALLAX
}
