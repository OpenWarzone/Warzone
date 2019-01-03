uniform sampler2D	u_DiffuseMap;
//uniform sampler2D	u_NormalMap;
uniform sampler2D	u_ScreenDepthMap;

//uniform mat4		u_invEyeProjectionMatrix;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec2		u_Dimensions;
uniform vec4		u_Local0;

varying vec2		var_ScreenTex;

#define		fvTexelSize (vec2(1.0) / u_Dimensions.xy)
#define		tex_offset fvTexelSize

// Sampling radius is in view space...
#ifdef FAST_HBAO
#define NUM_SAMPLING_DIRECTIONS 4
#define NUM_SAMPLING_STEPS 2
#else //!FAST_HBAO
#define NUM_SAMPLING_DIRECTIONS 8
#define NUM_SAMPLING_STEPS 4
#endif //FAST_HBAO

// sampling step is in texture space
//#define	SAMPLING_STEP 0.004
#define		SAMPLING_STEP 0.001

#define		SAMPLING_RADIUS 0.5
#define		TANGENT_BIAS 0.2

// Select a method...
//#define ATTENUATION_METHOD_1
#define		ATTENUATION_METHOD_2

#define		C_HBAO_STRENGTH 0.7
#define		C_HBAO_ZNEAR 0.0509804
#define		C_HBAO_ZFAR 2048.0//u_Local0.b//100.0

// Bumpize normal map...
//#define	C_HBAO_BUMPIZE1
//#define	C_HBAO_BUMPIZE2
#define		C_HBAO_BUMPIZE3

#define		C_HBAO_BUMPIZE_STRENGTH 0.02

// Debugging stuff...
//#define C_HBAO_DISPLAY_DEPTH
//#define C_HBAO_DISPLAY_NORMALS

float hbao_linearizeDepth ( float depth )
{
#if 0
	float d = depth;
	d /= C_HBAO_ZFAR - depth * C_HBAO_ZFAR + depth;
	return clamp(d, 0.0, 1.0);
#else
	return depth;
#endif
}

vec3 generateEnhancedNormal( vec2 fragCoord )
{// Generates a normal map with enhanced edges... Not so good for parallax...
	const float threshold = 0.085;

	vec3 rgb = textureLod(u_DiffuseMap, fragCoord, 0.0).rgb;
	vec3 bw = vec3(1.0, 1.0, 1.0);
	vec3 bw2 = vec3(1.0, 1.0, 1.0);

	vec3 rgbUp = textureLod(u_DiffuseMap, vec2(fragCoord.x,fragCoord.y+tex_offset.y), 0.0).rgb;
	vec3 rgbDown = textureLod(u_DiffuseMap, vec2(fragCoord.x,fragCoord.y-tex_offset.y), 0.0).rgb;
	vec3 rgbLeft = textureLod(u_DiffuseMap, vec2(fragCoord.x+tex_offset.x,fragCoord.y), 0.0).rgb;
	vec3 rgbRight = textureLod(u_DiffuseMap, vec2(fragCoord.x-tex_offset.x,fragCoord.y), 0.0).rgb;

	float rgbAvr = (rgb.r + rgb.g + rgb.b) / 3.;
	float rgbUpAvr = (rgbUp.r + rgbUp.g + rgbUp.b) / 3.;
	float rgbDownAvr = (rgbDown.r + rgbDown.g + rgbDown.b) / 3.;
	float rgbLeftAvr = (rgbLeft.r + rgbLeft.g + rgbLeft.b) / 3.;
	float rgbRightAvr = (rgbRight.r + rgbRight.g + rgbRight.b) / 3.;

	float dx = abs(rgbRightAvr - rgbLeftAvr);
	float dy = abs(rgbUpAvr - rgbDownAvr);

	if (dx > threshold)
		bw = vec3(1.0, 1.0, 1.0);
	else if (dy > threshold)
		bw = vec3(1.0, 1.0, 1.0);
	else
		bw = vec3(0.0, 0.0, 0.0);

	// o.5 + 0.5 * acts as a remapping function
	bw = 0.5 + 0.5*normalize( vec3(rgbRightAvr - rgbLeftAvr, 100.0*tex_offset.x, rgbUpAvr - rgbDownAvr) ).xzy;
	//bw = 0.5 + 0.5*normalize( vec3(rgbRightAvr - rgbLeftAvr, 100.0*tex_offset.x, rgbUpAvr - rgbDownAvr) ).xyz;

	//bw.g = 1.0-bw.g; // UQ1: Invert green on these...

	return bw * 2.0 - 1.0;
}

vec3 generateBumpyNormal( vec2 fragCoord )
{// Generates an extra bumpy normal map...
	const float x=1.0;
	const float y=1.0;

	float M =abs(length(textureLod(u_DiffuseMap, fragCoord + vec2(0., 0.)*tex_offset, 0.0).rgb) / 3.0);
	float L =abs(length(textureLod(u_DiffuseMap, fragCoord + vec2(x, 0.)*tex_offset, 0.0).rgb) / 3.0);
	float R =abs(length(textureLod(u_DiffuseMap, fragCoord + vec2(-x, 0.)*tex_offset, 0.0).rgb) / 3.0);	
	float U =abs(length(textureLod(u_DiffuseMap, fragCoord + vec2(0., y)*tex_offset, 0.0).rgb) / 3.0);;
	float D =abs(length(textureLod(u_DiffuseMap, fragCoord + vec2(0., -y)*tex_offset, 0.0).rgb) / 3.0);
	float X = ((R-M)+(M-L))*0.5;
	float Y = ((D-M)+(M-U))*0.5;

	//const float strength = 0.01;
	const float strength = C_HBAO_BUMPIZE_STRENGTH;
	vec4 N = vec4(normalize(vec3(X, Y, strength)), 1.0);
	//vec4 N = vec4(normalize(vec3(X, Y, strength)).xzy, 1.0);

	N.g = 1.0-N.g; // UQ1: Invert green on these...

	return N.xyz;
}

vec3 normal_from_depth(float depth, vec2 texcoords) {
	vec2 offset1 = vec2(0.0, fvTexelSize.y);
	vec2 offset2 = vec2(fvTexelSize.x, 0.0);

	float depth1 = textureLod(u_ScreenDepthMap, texcoords + offset1, 0.0).r;
	float depth2 = textureLod(u_ScreenDepthMap, texcoords + offset2, 0.0).r;

	depth1 = hbao_linearizeDepth(depth1);
	depth2 = hbao_linearizeDepth(depth2);

	vec3 p1 = vec3(offset1, depth1 - depth);
	vec3 p2 = vec3(offset2, depth2 - depth);

	vec3 normal = cross(p1, p2);
	normal.y = -normal.y;
	normal.z = -normal.z;

#if defined (C_HBAO_BUMPIZE3)
	vec3 norm2 = ((generateEnhancedNormal(texcoords) + generateBumpyNormal(texcoords)) / 2.0);
	normal *= norm2;
#elif defined (C_HBAO_BUMPIZE2)
	vec3 norm2 = generateEnhancedNormal(texcoords);
	normal *= norm2;
#elif defined (C_HBAO_BUMPIZE1)
	vec3 norm2 = generateBumpyNormal(texcoords);
	normal *= norm2;
#endif

	return normalize(normal);
}

vec3 SampleNormals(vec2 coord)  
{
	float depth = textureLod(u_ScreenDepthMap, coord, 0.0).r;

	depth = hbao_linearizeDepth(depth);

	return normal_from_depth(depth, coord);
}

mat4 CreateMatrixFromRows(vec4 r0, vec4 r1, vec4 r2, vec4 r3) {
	return mat4(r0, r1, r2, r3);
}

mat4 CreateMatrixFromCols(vec4 c0, vec4 c1, vec4 c2, vec4 c3) {
	return mat4(c0.x, c1.x, c2.x, c3.x,
		c0.y, c1.y, c2.y, c3.y,
		c0.z, c1.z, c2.z, c3.z,
		c0.w, c1.w, c2.w, c3.w);
}

#define M_PI		3.14159265358979323846

vec4 GetHBAO ( void )
{
#if defined (C_HBAO_DISPLAY_DEPTH)
//	if (u_Local0.a >= 2.0)
	{
		float depth = hbao_linearizeDepth(textureLod(u_ScreenDepthMap, var_ScreenTex.xy, 0.0).r);
		return vec4(depth, depth, depth, 1.0);
	}
#elif defined (C_HBAO_DISPLAY_NORMALS)
//	else if (u_Local0.a >= 1.0)
	{
	return vec4(SampleNormals(var_ScreenTex.xy).xyz * 0.5 + 0.5, 1.0);
	}
#else
//#if 1
	vec4 origColor = textureLod(u_DiffuseMap, var_ScreenTex.xy, 0.0);

	float start_Z = textureLod(u_ScreenDepthMap, var_ScreenTex.xy, 0.0).r; // returns value (z/w+1)/2

	start_Z = hbao_linearizeDepth(start_Z);

	if (start_Z > 0.999)// || start_Z <= 0.0)
	{// Ignore sky and loading/menu screens...
		return origColor;
	}

	float blend = ( ( clamp(start_Z, 0.0, 1.0) + (clamp(start_Z * start_Z, 0.0, 1.0) * 4.0)) / 5.0 );
	float HbaoMult = clamp(1.1 - blend, 0.0, 1.0);
	float invHbaoMult = 1.0 - HbaoMult;

	vec3 start_Pos = vec3(var_ScreenTex.xy, start_Z);
	vec3 ndc_Pos = (2.0 * start_Pos) - 1.0; // transform to normalized device coordinates xyz/w

//#define MIVP u_invEyeProjectionMatrix

	/*mat4 MIVP = CreateMatrixFromRows(vec4(0.0, 0.0, 0.0, 0.0), 
	vec4(1.0, 1.0, 1.0, 1.0), 
	vec4(0.5, 0.5, 0.5, 1.0), 
	vec4(-0.5, -0.5, 0.2, -1.2));*/

	mat4 MIVP = CreateMatrixFromCols(vec4(0.0, 0.0, 0.0, 0.0), 
		vec4(1.0, 1.0, 1.0, 1.0), 
		vec4(0.5, 0.5, 0.5, 1.0), 
		vec4(-0.5, -0.5, 0.2, -1.2));

	// reconstruct view space position
	vec4 unproject = MIVP * vec4(ndc_Pos, 1.0);
	vec3 viewPos = unproject.xyz / unproject.w; // 3d view space position P
	vec3 viewNorm = SampleNormals(var_ScreenTex.xy).xyz; // 3d view space normal N

	float total = 0.0;
	float sample_direction_increment = 2 * M_PI / NUM_SAMPLING_DIRECTIONS;

	for (int i = 0; i < NUM_SAMPLING_DIRECTIONS; i++) {
		// no jittering or randomization of sampling direction just yet
		float sampling_angle = i * sample_direction_increment; // azimuth angle theta in the paper
		vec2 sampleDir = vec2(cos(sampling_angle), sin(sampling_angle));

		// we will now march along sampleDir and calculate the horizon
		// horizon starts with the tangent plane to the surface, whose angle we can get from the normal
		float tangentAngle = acos(dot(vec3(sampleDir, 0), viewNorm)) - (0.5 * M_PI) + TANGENT_BIAS;
		float horizonAngle = tangentAngle;
		vec3 lastDiff = vec3(0.0, 0.0, 0.0);

		for (int j = 0; j < NUM_SAMPLING_STEPS; j++) 
		{// march along the sampling direction and see what the horizon is
			vec2 sampleOffset = float(j+1) * SAMPLING_STEP * sampleDir;
			vec2 offTex = var_ScreenTex.xy + sampleOffset;

			float off_start_Z = textureLod(u_ScreenDepthMap, offTex.xy, 0.0).r;

			off_start_Z = hbao_linearizeDepth(off_start_Z);

			vec3 off_start_Pos = vec3(offTex, off_start_Z);
			vec3 off_ndc_Pos = (2.0 * off_start_Pos) - 1.0;
			vec4 off_unproject = MIVP * vec4(off_ndc_Pos, 1.0);
			vec3 off_viewPos = off_unproject.xyz / off_unproject.w;

			// we now have the view space position of the offset point
			vec3 diff = off_viewPos.xyz - viewPos.xyz;

			if (length(diff) < SAMPLING_RADIUS) {
				// skip samples which are outside of our local sampling radius
				lastDiff = diff;
				float elevationAngle = atan(diff.z / length(diff.xy));
				horizonAngle = max(horizonAngle, elevationAngle);
			}
		}

		// the paper uses this attenuation but I like the other way better
#if defined(ATTENUATION_METHOD_1)
		float normDiff = length(lastDiff) / SAMPLING_RADIUS;
		float attenuation = 1.0 - normDiff*normDiff;
#elif defined(ATTENUATION_METHOD_2)
		float attenuation = 1.0 / (1 + length(lastDiff));
#else // Just in case someone doesn't define a method...
		float normDiff = length(lastDiff) / SAMPLING_RADIUS;
		float attenuation = 1.0 - normDiff*normDiff;
#endif //ATTENUATION_METHODS

		// now compare horizon angle to tangent angle to get ambient occlusion
		float occlusion = clamp(attenuation * (sin(horizonAngle) - sin(tangentAngle)), 0.0, 1.0);
		total += 1.0 - occlusion;
	}

	total /= NUM_SAMPLING_DIRECTIONS;

	total = pow(total, C_HBAO_STRENGTH);

	vec4 color = origColor;
	vec4 hbao = vec4(total, total, total, 1.0);

	//return hbao;

	// Based on depth...
	vec3 col1 = color.rgb;
	vec3 col2 = hbao.rgb * color.rgb;
	color.rgb = (col1 * invHbaoMult) + (col2 * HbaoMult);

	return color;
#endif
}

void main (void)
{
	gl_FragColor = GetHBAO();
}