//some stuff needed for kami-batch
varying vec2		var_TexCoords;
 
//make sure to have a u_Dimensions uniform set to the image size
uniform vec2		u_Dimensions;

uniform sampler2D	u_DiffuseMap;

vec2 tex_offset = vec2(1.0 / u_Dimensions.x, 1.0 / u_Dimensions.y);


//#define ENHANCED_NORMALS
//#define BUMPY_NORMALS
#define SAMPLEMAT_NORMALS

//#define HEIGHTMAP_ADD_NORMAL_BLUE


// This makes the darker areas less bumpy but I like it
#define USE_LINEAR_FOR_BUMPMAP

//#define EXPERIMENTAL_HEIGHTMAP

//
// Normal Map Stuff...
//

struct C_Sample
{
	vec3 vAlbedo;
	vec3 vNormal;
};
	
C_Sample SampleMaterial(const in vec2 vUV, sampler2D sampler,  const in vec2 vTextureSize, const in float fNormalScale)
{
	C_Sample result;
	
	vec2 vInvTextureSize = vec2(1.0) / vTextureSize;
	
	vec3 cSampleNegXNegY = texture2D(sampler, vUV + (vec2(-1.0, -1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXNegY = texture2D(sampler, vUV + (vec2( 0.0, -1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXNegY = texture2D(sampler, vUV + (vec2( 1.0, -1.0)) * vInvTextureSize.xy).rgb;
	
	vec3 cSampleNegXZerY = texture2D(sampler, vUV + (vec2(-1.0, 0.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXZerY = texture2D(sampler, vUV + (vec2( 0.0, 0.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXZerY = texture2D(sampler, vUV + (vec2( 1.0, 0.0)) * vInvTextureSize.xy).rgb;
	
	vec3 cSampleNegXPosY = texture2D(sampler, vUV + (vec2(-1.0,  1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXPosY = texture2D(sampler, vUV + (vec2( 0.0,  1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXPosY = texture2D(sampler, vUV + (vec2( 1.0,  1.0)) * vInvTextureSize.xy).rgb;

	// convert to linear	
	vec3 cLSampleNegXNegY = cSampleNegXNegY * cSampleNegXNegY;
	vec3 cLSampleZerXNegY = cSampleZerXNegY * cSampleZerXNegY;
	vec3 cLSamplePosXNegY = cSamplePosXNegY * cSamplePosXNegY;

	vec3 cLSampleNegXZerY = cSampleNegXZerY * cSampleNegXZerY;
	vec3 cLSampleZerXZerY = cSampleZerXZerY * cSampleZerXZerY;
	vec3 cLSamplePosXZerY = cSamplePosXZerY * cSamplePosXZerY;

	vec3 cLSampleNegXPosY = cSampleNegXPosY * cSampleNegXPosY;
	vec3 cLSampleZerXPosY = cSampleZerXPosY * cSampleZerXPosY;
	vec3 cLSamplePosXPosY = cSamplePosXPosY * cSamplePosXPosY;

	// Average samples to get albdeo colour
	result.vAlbedo = ( cLSampleNegXNegY + cLSampleZerXNegY + cLSamplePosXNegY 
		    	     + cLSampleNegXZerY + cLSampleZerXZerY + cLSamplePosXZerY
		    	     + cLSampleNegXPosY + cLSampleZerXPosY + cLSamplePosXPosY ) / 9.0;	
	
	vec3 vScale = vec3(0.3333);
	
	#ifdef USE_LINEAR_FOR_BUMPMAP
		
		float fSampleNegXNegY = dot(cLSampleNegXNegY, vScale);
		float fSampleZerXNegY = dot(cLSampleZerXNegY, vScale);
		float fSamplePosXNegY = dot(cLSamplePosXNegY, vScale);
		
		float fSampleNegXZerY = dot(cLSampleNegXZerY, vScale);
		float fSampleZerXZerY = dot(cLSampleZerXZerY, vScale);
		float fSamplePosXZerY = dot(cLSamplePosXZerY, vScale);
		
		float fSampleNegXPosY = dot(cLSampleNegXPosY, vScale);
		float fSampleZerXPosY = dot(cLSampleZerXPosY, vScale);
		float fSamplePosXPosY = dot(cLSamplePosXPosY, vScale);
	
	#else
	
		float fSampleNegXNegY = dot(cSampleNegXNegY, vScale);
		float fSampleZerXNegY = dot(cSampleZerXNegY, vScale);
		float fSamplePosXNegY = dot(cSamplePosXNegY, vScale);
		
		float fSampleNegXZerY = dot(cSampleNegXZerY, vScale);
		float fSampleZerXZerY = dot(cSampleZerXZerY, vScale);
		float fSamplePosXZerY = dot(cSamplePosXZerY, vScale);
		
		float fSampleNegXPosY = dot(cSampleNegXPosY, vScale);
		float fSampleZerXPosY = dot(cSampleZerXPosY, vScale);
		float fSamplePosXPosY = dot(cSamplePosXPosY, vScale);	
	
	#endif
	
	// Sobel operator - http://en.wikipedia.org/wiki/Sobel_operator
	
	vec2 vEdge;
	vEdge.x = (fSampleNegXNegY - fSamplePosXNegY) * 0.25 
			+ (fSampleNegXZerY - fSamplePosXZerY) * 0.5
			+ (fSampleNegXPosY - fSamplePosXPosY) * 0.25;

	vEdge.y = (fSampleNegXNegY - fSampleNegXPosY) * 0.25 
			+ (fSampleZerXNegY - fSampleZerXPosY) * 0.5
			+ (fSamplePosXNegY - fSamplePosXPosY) * 0.25;

	result.vNormal = normalize(vec3(vEdge * fNormalScale, 1.0));	
	
	return result;
}

vec4 generateEnhancedNormal( vec2 fragCoord )
{// Generates a normal map with enhanced edges... Not so good for parallax...
    const float threshold = 0.085;
   
    vec3 rgb = texture2D(u_DiffuseMap, fragCoord).rgb;
    vec3 bw = vec3(1);
    vec3 bw2 = vec3(1);

    vec3 rgbUp = texture2D(u_DiffuseMap, vec2(fragCoord.x,fragCoord.y+tex_offset.y)).rgb;
    vec3 rgbDown = texture2D(u_DiffuseMap, vec2(fragCoord.x,fragCoord.y-tex_offset.y)).rgb;
    vec3 rgbLeft = texture2D(u_DiffuseMap, vec2(fragCoord.x+tex_offset.x,fragCoord.y)).rgb;
    vec3 rgbRight = texture2D(u_DiffuseMap, vec2(fragCoord.x-tex_offset.x,fragCoord.y)).rgb;

    float rgbAvr = (rgb.r + rgb.g + rgb.b) / 3.;
    float rgbUpAvr = (rgbUp.r + rgbUp.g + rgbUp.b) / 3.;
    float rgbDownAvr = (rgbDown.r + rgbDown.g + rgbDown.b) / 3.;
    float rgbLeftAvr = (rgbLeft.r + rgbLeft.g + rgbLeft.b) / 3.;
    float rgbRightAvr = (rgbRight.r + rgbRight.g + rgbRight.b) / 3.;

    float dx = abs(rgbRightAvr - rgbLeftAvr);
    float dy = abs(rgbUpAvr - rgbDownAvr);
    
    if (dx > threshold)
        bw = vec3(1);
    else if (dy > threshold)
        bw = vec3(1);
    else
        bw = vec3(0);
    
    // o.5 + 0.5 * acts as a remapping function
    bw = 0.5 + 0.5*normalize( vec3(rgbRightAvr - rgbLeftAvr, 100.0*tex_offset.x, rgbUpAvr - rgbDownAvr) ).xzy;
    
    return vec4(bw,0);
}

vec4 generateBumpyNormal( vec2 fragCoord )
{// Generates an extra bumpy normal map...
	const float x=1.;
	const float y=1.;
	
	float M =abs(texture2D(u_DiffuseMap, fragCoord + vec2(0., 0.)*tex_offset).r); 
	float L =abs(texture2D(u_DiffuseMap, fragCoord + vec2(x, 0.)*tex_offset).r);
	float R =abs(texture2D(u_DiffuseMap, fragCoord + vec2(-x, 0.)*tex_offset).r);	
	float U =abs(texture2D(u_DiffuseMap, fragCoord + vec2(0., y)*tex_offset).r);
	float D =abs(texture2D(u_DiffuseMap, fragCoord + vec2(0., -y)*tex_offset).r);
	float X = ((R-M)+(M-L))*.5;
	float Y = ((D-M)+(M-U))*.5;
	
	const float strength =.01;
	vec4 N = vec4(normalize(vec3(X, Y, strength)), 1.0);

	vec4 col = vec4(N.xyz * 0.5 + 0.5,1.);
	return col;
}

vec4 GetNormal ( vec2 coord )
{
#if defined(ENHANCED_NORMALS)
	vec4 enhanced = generateEnhancedNormal(coord.xy);
#elif defined(BUMPY_NORMALS)
	vec4 bumpy = generateBumpyNormal(coord.xy);
#elif defined(SAMPLEMAT_NORMALS)
	// Mix methods...
	float fNormalScale = 10.0;//2.0;
	C_Sample sample = SampleMaterial(coord.xy, u_DiffuseMap,  u_Dimensions, fNormalScale);
	vec3 bumpMap = sample.vNormal;
#endif

#if defined(ENHANCED_NORMALS) && defined(BUMPY_NORMALS) && defined(SAMPLEMAT_NORMALS)
	normal = enhanced;
	normal.rgb += bumpMap.rgb;
	normal.rgb += bumpy.rgb;
	normal.rgb /= 3.0;
#elif defined(ENHANCED_NORMALS) && defined(BUMPY_NORMALS)
	vec4 normal = enhanced;
	normal.rgb += bumpy.rgb;
	normal.rgb /= 2.0;
#elif defined(ENHANCED_NORMALS) && defined(SAMPLEMAT_NORMALS)
	vec4 normal = enhanced;
	normal.rgb += bumpMap.rgb;
	normal.rgb /= 2.0;
#elif defined(BUMPY_NORMALS) && defined(SAMPLEMAT_NORMALS)
	vec4 normal = bumpy;
	normal.rgb += bumpMap.rgb;
	normal.rgb /= 2.0;
#elif defined(ENHANCED_NORMALS)
	vec4 normal = enhanced;
#elif defined(BUMPY_NORMALS)
	vec4 normal = bumpy;
#elif defined(SAMPLEMAT_NORMALS)
	vec4 normal = vec4(0.0);
	normal.rgb = bumpMap;
#endif

	return normal;
}


//
// Height Map Calculation...
//

float SampleHeight(vec2 t)
{// Provides enhanced parallax depths without stupid distortions... Also provides a nice backup specular map...
#if 0
	vec3 color = texture2D(u_DiffuseMap, t).rgb;
#define const_1 ( 16.0 / 255.0)
#define const_2 (255.0 / 219.0)
	vec3 color2 = ((color - const_1) * const_2);
#define const_3 ( 125.0 / 255.0)
#define const_4 (255.0 / 115.0)
	color = ((color - const_3) * const_4);

	// 1st half "color * color" darkens, 2nd half "* color * 5.0" increases the mids...
	color = clamp(color * color * (color * 5.0), 0.0, 1.0);

	vec3 orig_color = color + color2;

	// Lightens the new mixed version...
	orig_color = clamp(orig_color * 2.5, 0.0, 1.0);

	// Darkens the whole thing a litttle...
	float combined_color2 = orig_color.r + orig_color.g + orig_color.b;
	combined_color2 /= 4.0;

	// Returns inverse of the height. Result is mostly around 1.0 (so we don't stand on a surface far below us), with deep dark areas (cracks, edges, etc)...
	float height = clamp(1.0 - combined_color2, 0.0, 1.0);

	height = height * 0.5 + 0.5;
	height = clamp(height, 0.5, 0.9);

	return height;
#elif 1
	
	vec3 pixColor = texture2D(u_DiffuseMap, t).rgb;
	float brightness = length(pixColor);

	// Find the average color of the texture...
	vec3 avgColor = vec3(0.0);

	for (float p = 0.1; p < 1.0; p += 0.1)
	{// Grab an X covering the texture...
		avgColor += texture2D(u_DiffuseMap, vec2(p, p)).rgb;
		avgColor += texture2D(u_DiffuseMap, vec2(1.0-p, p)).rgb;
	}
	
	avgColor /= vec3(18.0);

	// Find the average of pixels near this pixel's location...
	float numAdded = 1.0;

	for (float x = -8.0; x <= 8.0; x += 1.0)
	{
		for (float y = -8.0; y <= 8.0; y += 1.0)
		{
			vec2 pos = vec2(t.x + (x * tex_offset.x), t.y + (y * tex_offset.y));
			
			vec3 color = texture2D(u_DiffuseMap, pos).rgb;

			if (pos.x >= 0.0 && pos.x <= 1.0 && pos.y >= 0.0 && pos.y <= 1.0)
			{
				pixColor += color;
				numAdded += 1.0;
			}
		}
	}

	pixColor /= vec3(numAdded);

	float aLen = length(avgColor) / 3.0;
	float pLen = length(pixColor) / 3.0;

	// Work out the difference between the color average and the close pixel average... That is height...
#define const_1 ( 36.0 / 255.0)
#define const_2 (255.0 / 219.0)

	float diff = 1.0;

	// Assumes that the average color is the average height... Any difference to the average is deeper...
	if (aLen <= pLen)
	{
		diff = (pLen - aLen) * 5.0;
		diff = 1.0 - diff;
		diff = clamp(diff, 0.0, 1.0); // Clamp to be sure...
		diff = ((clamp(diff - const_1, 0.0, 1.0)) * const_2); // amplify light/dark
		diff = clamp(diff, 0.0, 1.0); // Clamp to be sure...
	}
	else
	{
		diff = (aLen - pLen) * 5.0;
		diff = 1.0 - diff;
		diff = clamp(diff, 0.0, 1.0); // Clamp to be sure...
		diff = ((clamp(diff - const_1, 0.0, 1.0)) * const_2); // amplify light/dark
		diff = clamp(diff, 0.0, 1.0); // Clamp to be sure...
	}

	//return clamp(diff, 0.0, 1.0);
	//return clamp(diff * (1.5 - brightness), 0.0, 1.0);


	float avgBrightness = length(avgColor.rgb);

	bool invert = false;
	float maxbright = clamp(avgBrightness, 0.0, 1.0);

	if (avgBrightness >= 0.5)
	{
		invert = false;
	}
	else
	{
		invert = true;
	}

	float brightness2 = length(pixColor);
	float height = 0.0;

	if (brightness2 >= maxbright * 0.66666)
	{
		height = 1.0;
	}
	else
	{
		height = 0.0;
	}

	if (!invert)
		height = 1.0 - height;


	return (clamp(diff * (1.5 - brightness), 0.0, 1.0) + height + height) / 3.0;
#else
	// Find the average color of the texture...
	vec3 avgColor = vec3(0.0);

	for (float p = 0.1; p < 1.0; p += 0.1)
	{// Grab an X covering the texture...
		avgColor += texture2D(u_DiffuseMap, vec2(p, p)).rgb;
		avgColor += texture2D(u_DiffuseMap, vec2(1.0-p, p)).rgb;
	}
	
	avgColor /= vec3(18.0);
	float avgBrightness = length(avgColor.rgb);

	bool invert = false;
	float maxbright = clamp(avgBrightness, 0.0, 1.0);

	if (avgBrightness >= 0.5)
	{
		invert = false;
	}
	else
	{
		invert = true;
	}

	vec3 pixColor = texture2D(u_DiffuseMap, t).rgb;
	float brightness = length(pixColor);
	float height = 0.0;

	if (brightness >= maxbright * 0.66666)
	{
		height = 1.0;
	}
	else
	{
		height = 0.0;
	}

	if (invert)
		return height;
	else
		return 1.0 - height;
#endif
}


void main ( void )
{
	//
	// Normal Map RGB Channels...
	//

	vec4 normal = GetNormal( var_TexCoords.xy );

	//
	// Height Map Alpha Channel...
	//

#if defined(EXPERIMENTAL_HEIGHTMAP)
	normal.a = length(normal.rg) / 2.0;
#else //!defined(EXPERIMENTAL_HEIGHTMAP)
	normal.a = SampleHeight(var_TexCoords.xy);
#if defined(HEIGHTMAP_ADD_NORMAL_BLUE)
	normal.a += normal.b;
	normal.a /= 2.0;
#endif //defined(HEIGHTMAP_ADD_NORMAL_BLUE)
#endif //defined(EXPERIMENTAL_HEIGHTMAP)

	//normal.g = 1.0 - normal.g;
	normal.a = 1.0 - normal.a;

	gl_FragColor = normal;
}
