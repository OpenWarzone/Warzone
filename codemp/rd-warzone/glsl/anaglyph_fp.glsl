uniform sampler2D			u_DiffuseMap;
uniform sampler2D			u_ScreenDepthMap;

uniform vec4				u_ViewInfo; // zfar / znear, zfar
uniform vec2				u_Dimensions;
uniform vec4				u_Local0; // depthScale, 0, 0, 0
 uniform vec4				u_Local1; // AnaglyphType, AnaglyphMinDistance, AnaglyphMaxDistance, AnaglyphParallax

varying vec2				var_TexCoords;

#define texCoord			var_TexCoords

#define near				u_ViewInfo.x
#define far					u_ViewInfo.y
#define viewWidth			u_Dimensions.x
#define viewHeight			u_Dimensions.y

#define AnaglyphSeperation	u_Local0.r
#define AnaglyphRed			u_Local0.g
#define AnaglyphGreen		u_Local0.b
#define AnaglyphBlue		u_Local0.a

#define AnaglyphType		u_Local1.r
#define AnaglyphMinDistance u_Local1.g
#define AnaglyphMaxDistance u_Local1.b
#define AnaglyphParallax	u_Local1.a


float DepthToZPosition(float depth) {
	vec2 camerarange = vec2(AnaglyphMinDistance, AnaglyphMaxDistance);
	return camerarange.x / (camerarange.y - depth * (camerarange.y - camerarange.x)) * camerarange.y;
}

void main(void)
{
	if (AnaglyphType <= 1.0)
	{// Simple anaglyph 3D not using depth map...
		vec2 Depth = vec2(1.0f / viewWidth, 1.0f / viewHeight); // calculated from screen size now
		vec4 Anaglyph = textureLod(u_DiffuseMap, texCoord, 0.0);

		// Dubois anaglyph method. best option - not working...
		// Authors page: http://www.site.uottawa.ca/~edubois/anaglyph/
		// This method depends on the characteristics of the display device
		// and the anaglyph glasses.
		// The matrices below are taken from "A Uniform Metric for Anaglyph Calculation"
		// by Zhe Zhang and David F. McAllister, Proc. Electronic Imaging 2006, available
		// here: http://research.csc.ncsu.edu/stereographics/ei06.pdf
		// These matrices are supposed to work fine for LCD monitors and most red-cyan
		// glasses. See also the remarks in http://research.csc.ncsu.edu/stereographics/LS.pdf
		// (where slightly different values are used).
    
		vec4 color_l = textureLod(u_DiffuseMap, vec2(texCoord + (vec2(-AnaglyphParallax,0)*Depth)), 0.0);

		// Right Eye (Cyan)
		vec4 color_r = textureLod(u_DiffuseMap, vec2(texCoord + (vec2(AnaglyphParallax,0)*Depth)), 0.0);

		mat3x3 m0 = mat3x3(
                0.4155, -0.0458, -0.0545,
                0.4710, -0.0484, -0.0614,
                0.1670, -0.0258,  0.0128);
 
		mat3x3 m1 = mat3x3(
                -0.0109, 0.3756, -0.0651,
                -0.0365, 0.7333, -0.1286,
                -0.0060, 0.0111,  1.2968);
                    
		// calculate resulting pixel
		gl_FragColor = vec4((m0 * color_r.rgb) + (m1 * color_l.rgb), 1.0);
	} 
	else 
	{// Real anaglyph 3D using real depth map...
		vec2 ps = vec2(1.0f / viewWidth, 1.0f / viewHeight); // calculated from screen size now
		vec2 tc = texCoord;
	
		float depth = textureLod( u_ScreenDepthMap, tc, 0.0 ).x;
		depth=pow(depth, 255);
		//gl_FragColor = vec4(vec3((depth*AnaglyphParallax)*(depth*AnaglyphParallax)), 1.0);
		//gl_FragColor = vec4(vec3(depth), 1.0);
		//return;

		//float lineardepth = DepthToZPosition( depth );
		float lineardepth = depth;
	
		lineardepth*=AnaglyphParallax;
		float shift=lineardepth;

		vec4 color_l = textureLod(u_DiffuseMap, vec2(tc.x-ps.x*shift,tc.y), 0.0);
		vec4 color_r = textureLod(u_DiffuseMap, vec2(tc.x+ps.x*shift,tc.y), 0.0);

		// Dubois anaglyph method. Best option...
		// Authors page: http://www.site.uottawa.ca/~edubois/anaglyph/
		// This method depends on the characteristics of the display device
		// and the anaglyph glasses.
		// The matrices below are taken from "A Uniform Metric for Anaglyph Calculation"
		// by Zhe Zhang and David F. McAllister, Proc. Electronic Imaging 2006, available
		// here: http://research.csc.ncsu.edu/stereographics/ei06.pdf
		// These matrices are supposed to work fine for LCD monitors and most red-cyan
		// glasses. See also the remarks in http://research.csc.ncsu.edu/stereographics/LS.pdf
		// (where slightly different values are used).
    
		mat3x3 m0 = mat3x3(
                0.4155, -0.0458, -0.0545,
                0.4710, -0.0484, -0.0614,
                0.1670, -0.0258,  0.0128);
 
		mat3x3 m1 = mat3x3(
                -0.0109, 0.3756, -0.0651,
                -0.0365, 0.7333, -0.1286,
                -0.0060, 0.0111,  1.2968);
                    
		gl_FragColor = vec4((m0 * color_r.rgb) + (m1 * color_l.rgb), 1.0);
	}
}
